#ifndef ACTIVITY_H

#define ACTIVITY_H

typedef enum{
    Running,Stopped
}state;

typedef struct{
    char* command ;
    int   pid ;
    state state;    
}job;

void activity();
void update();
void ping();

extern job* jobs[];
extern int job_count;

#endif