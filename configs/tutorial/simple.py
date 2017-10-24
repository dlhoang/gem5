#import all gem5's objects
import m5
from m5.objects import *  #imports all SimObjects

#instantiate a system
#"System" is a Python class wrapper for the System C++ SimObject

system = System()


#Initialize a clock and voltage domain
#clk_domain is a parameter of the System SimObject
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
#gem5 is smart enough to automatically convert units

#Each clock domain needs a voltage domain
#Use the default (1 volt)
system.clk_domain.voltage_domain = VoltageDomain()

#let's set up a memory system
system.mem_mode = 'timing'
#You want to use timing for all your simluations -
#other options are 'atomic' - don't wait for any
#time to pass when you're doing memory operations
#and 'functional' (debug) - use introspection
#use timing if you want timing results out of your simulator

#All systems need memory!
system.mem_ranges = [AddrRange('512MB')]

#Let's create a CPU
system.cpu = TimingSimpleCPU()
#everything executes in a single cycle except memory

#Need a memory bus
system.membus = SystemXBar()


system.cache = BaseCache()

#Hook up CPU to memory system
system.cpu.icache_port = system.membus.slave
system.cpu.dcache_port = system.membus.slave

#Next, some stuff to get things right
system.cpu.createInterruptController()
system.cpu.interrupts[0].pio = system.membus.master
system.cpu.interrupts[0].int_master = system.membus.slave
system.cpu.interrupts[0].int_slave = system.membus.master

system.system_port = system.membus.slave

#Finally, let's make the memory controller
system.mem_ctrl = DDR3_1600_8x8()

#Set up physical memory ranges
system.mem_ctrl.range = system.mem_ranges[0]

#Connect memory to bus
system.mem_ctrl.port = system.membus.master
#memory controller is connected to same bus we connected the cpu to

#Now tell the system what we want it to do
#create a process
process = Process()

#this process has a command
#this binary already exists in the files
process.cmd = ['tests/test-progs/hello/bin/x86/linux/hello']
#set the cpu to run this process
system.cpu.workload = process

#create the cpu threads
system.cpu.createThreads()

#Now, we're almost done
#Create a root object
root = Root(full_system = False, system = system)

#Instantiate all of the C++ objects
#Tell gem5's python API that we want to actually make all the C++ objects
m5.instantiate()
#Now you can no longer make any new SimObjects in your python config file

#We're ready to run
#Remember this is a normal python file
print "Beginning simulation!"

exit_event = m5.simulate()

print "Exiting @ tick %i because %s" % (m5.curTick(), exit_event.getCause())





