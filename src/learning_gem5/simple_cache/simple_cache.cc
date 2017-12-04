/*
 * Copyright (c) 2017 Jason Lowe-Power
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Jason Lowe-Power
 */

#include "learning_gem5/simple_cache/simple_cache.hh"

#include "base/random.hh"
#include "debug/Insert.hh"
#include "debug/SimpleCache.hh"
#include "sim/system.hh"

using namespace std;

SimpleCache::LRUCache cache(1);

SimpleCache::SimpleCache(SimpleCacheParams *params) :
    MemObject(params),
    latency(params->latency),
    blockSize(params->system->cacheLineSize()),
    capacity(params->size / blockSize),
    memPort(params->name + ".mem_side", this),
    blocked(false), outstandingPacket(nullptr), waitingPortId(-1)
{
    // Since the CPU side ports are a vector of ports, create an instance of
    // the CPUSidePort for each connection. This member of params is
    // automatically created depending on the name of the vector port and
    // holds the number of connections to this port name
    for (int i = 0; i < params->port_cpu_side_connection_count; ++i) {
        cpuPorts.emplace_back(name() + csprintf(".cpu_side[%d]", i), i, this);
    }

    cache.setCapacity(capacity);
}

BaseMasterPort&
SimpleCache::getMasterPort(const std::string& if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    // This is the name from the Python SimObject declaration in SimpleCache.py
    if (if_name == "mem_side") {
        return memPort;
    } else {
        // pass it along to our super class
        return MemObject::getMasterPort(if_name, idx);
    }
}

BaseSlavePort&
SimpleCache::getSlavePort(const std::string& if_name, PortID idx)
{
    // This is the name from the Python SimObject declaration (SimpleMemobj.py)
    if (if_name == "cpu_side" && idx < cpuPorts.size()) {
        // We should have already created all of the ports in the constructor
        return cpuPorts[idx];
    } else {
        // pass it along to our super class
        return MemObject::getSlavePort(if_name, idx);
    }
}

void
SimpleCache::CPUSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the cache is blocking.

    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    DPRINTF(SimpleCache, "Sending %s to CPU\n", pkt->print());
    if (!sendTimingResp(pkt)) {
        DPRINTF(SimpleCache, "failed!\n");
        blockedPacket = pkt;
    }
}

AddrRangeList
SimpleCache::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

void
SimpleCache::CPUSidePort::trySendRetry()
{
    if (needRetry && blockedPacket == nullptr) {
        // Only send a retry if the port is now completely free
        needRetry = false;
        DPRINTF(SimpleCache, "Sending retry req.\n");
        sendRetryReq();
    }
}

void
SimpleCache::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    // Just forward to the cache.
    return owner->handleFunctional(pkt);
}

bool
SimpleCache::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(SimpleCache, "Got request %s\n", pkt->print());

    if (blockedPacket || needRetry) {
        // The cache may not be able to send a reply if this is blocked
        DPRINTF(SimpleCache, "Request blocked\n");
        needRetry = true;
        return false;
    }
    // Just forward to the cache.
    if (!owner->handleRequest(pkt, id)) {
        DPRINTF(SimpleCache, "Request failed\n");
        // stalling
        needRetry = true;
        return false;
    } else {
        DPRINTF(SimpleCache, "Request succeeded\n");
        return true;
    }
}

void
SimpleCache::CPUSidePort::recvRespRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    DPRINTF(SimpleCache, "Retrying response pkt %s\n", pkt->print());
    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);

    // We may now be able to accept new packets
    trySendRetry();
}

void
SimpleCache::MemSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the cache is blocking.

    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

bool
SimpleCache::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    // Just forward to the cache.
    return owner->handleResponse(pkt);
}

void
SimpleCache::MemSidePort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}

void
SimpleCache::MemSidePort::recvRangeChange()
{
    owner->sendRangeChange();
}

bool
SimpleCache::handleRequest(PacketPtr pkt, int port_id)
{
    if (blocked) {
        // There is currently an outstanding request so we can't respond. Stall
        return false;
    }

    DPRINTF(SimpleCache, "Got request for addr %#x\n", pkt->getAddr());

    // This cache is now blocked waiting for the response to this packet.
    blocked = true;

    // Store the port for when we get the response
    assert(waitingPortId == -1);
    waitingPortId = port_id;

    // Schedule an event after cache access latency to actually access
    schedule(new AccessEvent(this, pkt), clockEdge(latency));

    return true;
}

bool
SimpleCache::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(SimpleCache, "Got response for addr %#x\n", pkt->getAddr());

    // For now assume that inserts are off of the critical path and don't count
    // for any added latency.
    insert(pkt);

    missLatency.sample(curTick() - missTime);

    if (outstandingPacket != nullptr) {
        DPRINTF(SimpleCache, "Copying data from new packet to old\n");
        // We had to upgrade a previous packet. We can functionally deal with
        // the cache access now. It better be a hit.
        bool hit M5_VAR_USED = accessFunctional(outstandingPacket);
        panic_if(!hit, "Should always hit after inserting");
        outstandingPacket->makeResponse();
        delete pkt; // We may need to delay this, I'm not sure.
        pkt = outstandingPacket;
        outstandingPacket = nullptr;
    } // else, pkt contains the data it needs

    sendResponse(pkt);

    return true;
}

void SimpleCache::sendResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(SimpleCache, "Sending resp for addr %#x\n", pkt->getAddr());

    int port = waitingPortId;

    // The packet is now done. We're about to put it in the port, no need for
    // this object to continue to stall.
    // We need to free the resource before sending the packet in case the CPU
    // tries to send another request immediately (e.g., in the same callchain).
    blocked = false;
    waitingPortId = -1;

    // Simply forward to the memory port
    cpuPorts[port].sendPacket(pkt);

    // For each of the cpu ports, if it needs to send a retry, it should do it
    // now since this memory object may be unblocked now.
    for (auto& port : cpuPorts) {
        port.trySendRetry();
    }
}

void
SimpleCache::handleFunctional(PacketPtr pkt)
{
    if (accessFunctional(pkt)) {
        pkt->makeResponse();
    } else {
        memPort.sendFunctional(pkt);
    }
}

void
SimpleCache::accessTiming(PacketPtr pkt)
{
    bool hit = accessFunctional(pkt);

    DPRINTF(SimpleCache, "%s for packet: %s\n", hit ? "Hit" : "Miss",
            pkt->print());

    if (hit) {
        // Respond to the CPU side
        hits++; // update stats
        DDUMP(SimpleCache, pkt->getConstPtr<uint8_t>(), pkt->getSize());
        pkt->makeResponse();
        sendResponse(pkt);
        if (LRU) {
            cache.rearrange_list(pkt->getAddr());
        }
    } else {
        misses++; // update stats
        missTime = curTick();
        // Forward to the memory side.
        // We can't directly forward the packet unless it is exactly the size
        // of the cache line, and aligned. Check for that here.
        Addr addr = pkt->getAddr();
        Addr block_addr = pkt->getBlockAddr(blockSize);
        unsigned size = pkt->getSize();
        if (addr == block_addr && size == blockSize) {
            // Aligned and block size. We can just forward.
            DPRINTF(SimpleCache, "forwarding packet\n");
            memPort.sendPacket(pkt);
        } else {
            DPRINTF(SimpleCache, "Upgrading packet to block size\n");
            panic_if(addr - block_addr + size > blockSize,
                     "Cannot handle accesses that span multiple cache lines");
            // Unaligned access to one cache block
            assert(pkt->needsResponse());
            MemCmd cmd;
            if (pkt->isWrite() || pkt->isRead()) {
                // Read the data from memory to write into the block.
                // We'll write the data in the cache (i.e., a writeback cache)
                cmd = MemCmd::ReadReq;
            } else {
                panic("Unknown packet type in upgrade size");
            }

            // Create a new packet that is blockSize
            PacketPtr new_pkt = new Packet(pkt->req, cmd, blockSize);
            new_pkt->allocate();

            // Should now be block aligned
            assert(new_pkt->getAddr() == new_pkt->getBlockAddr(blockSize));

            // Save the old packet
            outstandingPacket = pkt;

            DPRINTF(SimpleCache, "forwarding packet\n");
            memPort.sendPacket(new_pkt);
        }
    }
}

bool
SimpleCache::accessFunctional(PacketPtr pkt)
{
    Addr block_addr = pkt->getBlockAddr(blockSize);
    auto it = cacheStore.find(block_addr);
    if (it != cacheStore.end()) {
        if (pkt->isWrite()) {
            // Write the data into the block in the cache
            pkt->writeDataToBlock(it->second, blockSize);
        } else if (pkt->isRead()) {
            // Read the data out of the cache block into the packet
            pkt->setDataFromBlock(it->second, blockSize);
        } else {
            panic("Unknown packet type!");
        }
        return true;
    }
    return false;
}

void
SimpleCache::insert(PacketPtr pkt)
{
    DPRINTF(Insert, "========  INSERT  =================\n");
    // The packet should be aligned.
    assert(pkt->getAddr() ==  pkt->getBlockAddr(blockSize));
    // The address should not be in the cache
    assert(cacheStore.find(pkt->getAddr()) == cacheStore.end());
    // The pkt should be a response
    assert(pkt->isResponse());

    if (cacheStore.size() >= capacity) {
        DPRINTF(Insert, "cacheStore.size() %d  capacity %d\n",
                        cacheStore.size(), capacity);
        // Select random thing to evict. This is a little convoluted since we
        // are using a std::unordered_map. See http://bit.ly/2hrnLP2
        int bucket;
        int bucket_size;
        int number = 0;
        Addr addressToDelete;

        if (FIFO) {
            addressToDelete = deleteFIFO();
            auto block = cacheStore.find(addressToDelete);
            DPRINTF(Insert, "Removing addr %#x\n", block->first);

            // Write back the data.
            // Create a new request-packet pair
            RequestPtr req = new Request(block->first, blockSize, 0, 0);
            PacketPtr new_pkt = new Packet(req,
                                           MemCmd::WritebackDirty, blockSize);
            new_pkt->dataDynamic(block->second); // This will be deleted later

            DPRINTF(Insert, "Writing packet back %s\n", pkt->print());
            // Send the write to memory
            memPort.sendTimingReq(new_pkt);

            // Delete this entry
            cacheStore.erase(block->first);
        }

        else if (FILO) {
            addressToDelete = deleteFILO();
            auto block = cacheStore.find(addressToDelete);
            DPRINTF(Insert, "Removing addr %#x\n", block->first);

            // Write back the data.
            // Create a new request-packet pair
            RequestPtr req = new Request(block->first, blockSize, 0, 0);
            PacketPtr new_pkt = new Packet(req,
                                           MemCmd::WritebackDirty, blockSize);
            new_pkt->dataDynamic(block->second); // This will be deleted later

            DPRINTF(Insert, "Writing packet back %s\n", pkt->print());
            // Send the write to memory
            memPort.sendTimingReq(new_pkt);

            // Delete this entry
            cacheStore.erase(block->first);

        }

        else if (SEQUENTIAL) {
            do {
                bucket = number++;
                DPRINTF(Insert, "Bucket %d\n", bucket);
            } while ( (bucket_size = cacheStore.bucket_size(bucket)) == 0 );
            auto block = std::next(cacheStore.begin(bucket), 0);
            DPRINTF(Insert, "Removing addr %#x\n", block->first);

            // Write back the data.
            // Create a new request-packet pair
            RequestPtr req = new Request(block->first, blockSize, 0, 0);
            PacketPtr new_pkt = new Packet(req,
                                           MemCmd::WritebackDirty, blockSize);
            new_pkt->dataDynamic(block->second); // This will be deleted later

            DPRINTF(Insert, "Writing packet back %s\n", pkt->print());
            // Send the write to memory
            memPort.sendTimingReq(new_pkt);

            // Delete this entry
            cacheStore.erase(block->first);
        }

        else if (RANDOM) {
            do {
                //Get a random bucket
                //bucket_count returns the number of
                //buckets in the unordered_map
                bucket = random_mt.random(0,
                                          (int)cacheStore.bucket_count() - 1);
                DPRINTF(Insert, "Bucket: %d\n", bucket);
                DPRINTF(Insert, "Number of buckets: %d\n",
                                cacheStore.bucket_count());
                DPRINTF(Insert, "Max size: %d\n", cacheStore.max_size());
                DPRINTF(Insert, "Elements in bucket %d: %d\n",
                                bucket, cacheStore.bucket_size(bucket));
                //If that bucket has no elements, get another bucket
                //bucket_size(n) returns the number of elements in bucket n
            } while ( (bucket_size = cacheStore.bucket_size(bucket)) == 0 );
            //cacheStore.begin(bucket) is an iterator to the elements
            //in the bucket
            //next uses this iterator to advance to a random next element
            //bucket_size returns the number of elements in the bucket
            auto block = std::next(cacheStore.begin(bucket),
                                   random_mt.random(0, bucket_size - 1));
            //iterator block points to an element in the map
            //an element is a <key, value> pair
            //block->first gets the key
            //block->second gets the value
            DPRINTF(Insert,
                            "Removing addr %#x which contains value: %#x\n",
                            block->first, block->second);

            // Write back the data.
            // Create a new request-packet pair
            RequestPtr req = new Request(block->first, blockSize, 0, 0);
            PacketPtr new_pkt = new Packet(req,
                                           MemCmd::WritebackDirty, blockSize);
            new_pkt->dataDynamic(block->second); // This will be deleted later

            DPRINTF(Insert, "Writing packet back %s\n", pkt->print());
            // Send the write to memory
            memPort.sendTimingReq(new_pkt);

            // Delete this entry
            cacheStore.erase(block->first);
        }

    }

    //Print the cacheStore
    DPRINTF(Insert, "-------------  CacheStore -------------\n");
    for (auto it : cacheStore)
       DPRINTF(Insert, "first: %x second %x\n", it.first, it.second);
    DPRINTF(Insert, "-------------  End CacheStore -------------\n");

    DPRINTF(Insert, "Inserting %s\n", pkt->print());
    //DDUMP(Insert, pkt->getConstPtr<uint8_t>(), blockSize);

    // Allocate space for the cache block data
    uint8_t *data = new uint8_t[blockSize];

    if (FIFO)
    {
        // Insert into linked list
        insertFIFO(pkt->getAddr());
    }

    else if (FILO)
    {
        // Insert into linked list
        insertFILO(pkt->getAddr());
    }

    // Insert into LRU list
    else if (LRU)
    {
        // put new entry in LRU cache
        LRUNode *temp = cache.put(pkt->getAddr(), data);

        // filled capacity, tail gets returned
        if (cacheStore.size() >= capacity) {
            DPRINTF(SimpleCache, "%#x\n", temp->key);
            Addr addressToDelete = temp->key;

            if (cacheStore.find(addressToDelete) == cacheStore.end()) {
                DPRINTF(SimpleCache, "NOT FOUND IN CACHESTORE\n");
            }
            auto block = cacheStore.find(addressToDelete);
            DPRINTF(SimpleCache, "Removing addr %#x\n", block->first);

            // Write back the data.
            // Create a new request-packet pair
            RequestPtr req = new Request(block->first, blockSize, 0, 0);
            PacketPtr new_pkt = new Packet(req,
                                           MemCmd::WritebackDirty, blockSize);
            new_pkt->dataDynamic(block->second); // This will be deleted later

            DPRINTF(Insert, "Writing packet back %s\n", pkt->print());
            // Send the write to memory
            memPort.sendTimingReq(new_pkt);

            // Delete this entry
            cacheStore.erase(block->first);
        }
    }

    // Insert the data and address into the cache store
    cacheStore[pkt->getAddr()] = data;
    DPRINTF(SimpleCache, "INSERTING INTO CACHESTORE: %#x\n", pkt->getAddr());

    // Write the data into the cache
    pkt->writeDataToBlock(data, blockSize);

    DPRINTF(Insert, "===========  END INSERT  ==========\n\n");
}

// Adds to head of the list
void
SimpleCache::insertFIFO(Addr address)
{
    node *temp = new node;
    temp->address = address;
    temp->next = NULL;
    temp->prev = NULL;

    if (head == NULL)
    {
        //DPRINTF(Insert, "head == NULL\n");
        head = temp;
        tail = temp;
    }
    else
    {
        temp->next = head;
        head->prev = temp;
        head = temp;
    }
}

// Deletes from the tail of the list
Addr
SimpleCache::deleteFIFO()
{
    assert(tail != NULL);

    node *curr = tail;
    Addr address = curr->address;

    node *temp = tail->prev;
    if (temp != NULL)
    {
        temp->next = NULL;
    }
    tail = temp;

    if (tail == NULL)
    {
        head = NULL;
    }
    delete curr;

    return address;
}

// Adds to head of the list
void
SimpleCache::insertFILO(Addr address)
{
    node *temp = new node;
    temp->address = address;
    temp->next = NULL;

    if (filoHead == NULL)
    {
        filoHead = temp;
    }
    else
    {
        temp->next = filoHead;
        filoHead = temp;
    }
}

// Deletes from head of the list
Addr
SimpleCache::deleteFILO()
{
    assert(filoHead != NULL);

    node *curr = filoHead;
    Addr address = curr->address;

    filoHead = filoHead->next;

    delete curr;
    return address;
}

AddrRangeList
SimpleCache::getAddrRanges() const
{
    //DPRINTF(SimpleCache, "Sending new ranges\n");
    // Just use the same ranges as whatever is on the memory side.
    return memPort.getAddrRanges();
}

void
SimpleCache::sendRangeChange() const
{
    for (auto& port : cpuPorts) {
        port.sendRangeChange();
    }
}

void
SimpleCache::regStats()
{
    // If you don't do this you get errors about uninitialized stats.
    MemObject::regStats();

    hits.name(name() + ".hits")
        .desc("Number of hits")
        ;

    misses.name(name() + ".misses")
        .desc("Number of misses")
        ;

    missLatency.name(name() + ".missLatency")
        .desc("Ticks for misses to the cache")
        .init(16) // number of buckets
        ;

    hitRatio.name(name() + ".hitRatio")
        .desc("The ratio of hits to the total accesses to the cache")
        ;

    hitRatio = hits / (hits + misses);

}


SimpleCache*
SimpleCacheParams::create()
{
    return new SimpleCache(this);
}

bool
SimpleCache::DoublyLinkedList::isEmpty() {
    return rear == NULL;
}

SimpleCache::LRUNode*
SimpleCache::DoublyLinkedList::add_page_to_head(Addr key) {
    LRUNode *page = new LRUNode(key);
    if (!front && !rear)
    {
        front = rear = page;
    }
    else
    {
        page->next = front;
        front->prev = page;
        front = page;
    }
    return page;
}

void
SimpleCache::DoublyLinkedList::move_page_to_head(LRUNode *page) {
    if (page==front) {
        return;
    }
    if (page == rear) {
        rear = rear->prev;
        rear->next = NULL;
    }
    else {
        page->prev->next = page->next;
        page->next->prev = page->prev;
    }

    page->next = front;
    page->prev = NULL;
    front->prev = page;
    front = page;
}

SimpleCache::LRUNode *
SimpleCache::DoublyLinkedList::remove_rear_page() {
    LRUNode *retNode = new LRUNode(0);
    if (isEmpty())
    {
        return NULL;
    }
    if (front == rear)
    {
        retNode->key = rear->key;
        //delete rear;
        front = rear = NULL;
    }
    else
    {
        //LRUNode *temp = rear;
        retNode->key = rear->key;
        rear = rear->prev;
        rear->next = NULL;
        //delete temp;
    }
    return retNode;
}

SimpleCache::LRUNode*
SimpleCache::DoublyLinkedList::get_rear_page() {
    return rear;
}


SimpleCache::LRUCache::LRUCache(int capacity) {
    this->capacity = capacity;
    size = 0;
    pageList = new DoublyLinkedList();
    pageMap = map<Addr, LRUNode*>();
}

//uint8_t*
//SimpleCache::LRUCache::get(Addr key) {
    //if (pageMap.find(key)==pageMap.end()) {
        //return NULL;
    //}
    //uint8_t *val = pageMap[key]->value;

    // move the page to front
    //pageList->move_page_to_head(pageMap[key]);
    //return val;
//}

SimpleCache::LRUNode *
SimpleCache::LRUCache::put(Addr key, uint8_t *value) {

    LRUNode *temp = new LRUNode(key);

//    if (pageMap.find(key)!=pageMap.end()) {
//        // if key already present, update value and move page to head
//        //pageMap[key]->value = value;
//        pageList->move_page_to_head(pageMap[key]);
//        return temp;
//    }

    if (size == capacity) {
        // remove rear page
        Addr k = pageList->get_rear_page()->key;
        pageMap.erase(k);
        temp = pageList->remove_rear_page();
        size--;
    }

    // add new page to head to Queue
    LRUNode *page = pageList->add_page_to_head(key);
    size++;
    pageMap[key] = page;

    return temp;
}

Addr
SimpleCache::LRUCache::rearrange_list(Addr key) {
    auto block = pageMap.find(key);
    if (block!=pageMap.end()) {
        // if key already present, update value and move page to head
        pageList->move_page_to_head(pageMap[key]);
        return block->first;
    }
    return 0;
}

void
SimpleCache::LRUCache::setCapacity(int capacity) {
    this->capacity = capacity;
}
