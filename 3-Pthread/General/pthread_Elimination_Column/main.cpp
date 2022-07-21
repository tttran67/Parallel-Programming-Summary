#include <iostream>
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/time.h>
#include<arm_neon.h>
#define NUM_THREADS 7
using namespace std;
const int n = 2000;
float A[n][n];

//��ʼ������A
void ReSet(){
    for(int i = 0;i < n; i++){
        for(int j = 0;j < i; j++)
            A[i][j] = 0;
        A[i][i] = 1.0;
        for(int j = i + 1;j < n; j++)
            A[i][j] = rand() % 100;
    }
    for(int i = 0;i < n; i++){
        int k1 = rand() % n;
        int k2 = rand() % n;
        for(int j = 0;j < n; j++){
            A[i][j] += A[0][j];
            A[k1][j] += A[k2][j];
        }
    }
}
//�߳����ݽṹ����
typedef struct{
    int t_id;//�߳�id
}threadParam_t;
//barrier����
pthread_barrier_t barrier_Division;
pthread_barrier_t barrier_Elimination;
//�̺߳�������
void *threadFunc(void *param){
    float32x4_t vx = vmovq_n_f32(0);
    float32x4_t vaij = vmovq_n_f32(0);
    float32x4_t vaik = vmovq_n_f32(0);
    float32x4_t vakj = vmovq_n_f32(0);
    threadParam_t *p = (threadParam_t*)param;
    int t_id = p->t_id;

    for(int k = 0;k < n;++k){
        int j = k + t_id + 1;
        for(;j < n; j += NUM_THREADS){
            A[k][j] = A[k][j] / A[k][k];
        }
        A[k][k] = 1.0;

        //��һ��ͬ����
        pthread_barrier_wait(&barrier_Division);

        //we divide the procedure by column.
        for(int i = k + 1;i < n;i ++){
            //��ȥ
            for(int j = k + 1 + t_id;j < n; j += NUM_THREADS){
                A[i][j] = A[i][j] - A[i][k] * A[k][j];
            }

        }
        //�ڶ���ͬ����
        pthread_barrier_wait(&barrier_Elimination);
        if(t_id == 0)
            for(int i = k + 1;i < n; i++)
                 A[i][k] = 0.0;
    }
    pthread_exit(NULL);

}
int main()
{
    //��ʼ��
    ReSet();
    struct timeval head;
    struct timeval tail;
    //��ʼ��barrier
    pthread_barrier_init(&barrier_Division,NULL,NUM_THREADS);
    pthread_barrier_init(&barrier_Elimination,NULL,NUM_THREADS);
    //�����߳�
    pthread_t handles[NUM_THREADS];//������Ӧ��handle
    threadParam_t param[NUM_THREADS];//������Ӧ���߳����ݽṹ
    gettimeofday(&head,NULL);
    for(int t_id = 0;t_id < NUM_THREADS;++t_id){
        param[t_id].t_id = t_id;
        pthread_create(&handles[t_id],NULL,threadFunc,(void*)&param[t_id]);
    }

    for(int t_id = 0;t_id < NUM_THREADS;++t_id){
        pthread_join(handles[t_id],NULL);
    }
    gettimeofday(&tail,NULL);
    cout<<"N: "<<n<<" Time: "<<(tail.tv_sec-head.tv_sec)*1000.0+(tail.tv_usec-head.tv_usec)/1000.0<<"ms";
    //�������е�barrier
    pthread_barrier_destroy(&barrier_Division);
    pthread_barrier_destroy(&barrier_Elimination);

    return 0;
}
