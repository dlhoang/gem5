#include <stdio.h>
#include <string.h>

#define NAME_LEN 20
#define S1_ID 1
#define S1_GPA 3.0
#define S2_ID 4
#define S2_GPA 3.5

typedef struct {
   int id;
   char name[NAME_LEN + 1];
   double gpa;
} Student;

void CloneStudent(Student *s1, Student *s2) {
   *s1 = *s2; // Base: 5, Surcharge: 0
}

int IsEqual(Student *sa, Student *sb) {
    return sa->id == sb->id && sa->gpa == sb->gpa
          && strcmp(sa->name, sb->name) == 0;
}

int main() {
   Student s1 = {S1_ID, "Bob", S1_GPA}, s2 = {S2_ID, "Sue", S2_GPA};

   CloneStudent(&s1, &s2);

   if (IsEqual(&s1, &s2)) {
      printf("Successfuly cloned a student");
   }
   else {
      printf("Failed to clone a student");
   }

   return 0;
}
