#include "Thread.h"
#include "Simulator.h"
#include <assert.h>

Thread::Thread(char* threadName){
    
	name = threadName;
	stackTop = NULL;
    stack = NULL;
    for (int i = 0; i < MachineStateSize; i++) {
		machineState[i] = NULL;
    }
}

Thread::~Thread(){  
    assert(this != simulator->currentThread);
    if (stack != NULL)
		delete [] stack;
}


void Thread::Fork(VoidFunctionPtr func, void *arg){
    StackAllocate(func, arg);
    simulator->ReadyToRun(this);	
}    

// this is put at the top of the execution stack, for detecting stack overflows
const int STACK_FENCEPOST = 0xdedbeef;

void Thread::CheckOverflow(){
    if (stack != NULL) {
		assert(*stack == STACK_FENCEPOST);
   }
}


void Thread::Begin (){
    assert(this == simulator->currentThread);
    simulator->CheckToBeDestroyed();
}

void Thread::Finish (){
   
    assert(this == simulator->currentThread);
    Ssleep(true);				
}


void Thread::yield (){
    Thread *nextThread;
	
    assert(this == simulator->currentThread);

    nextThread = simulator->FindNextToRun();

    if (nextThread != NULL) {
		simulator->ReadyToRun(this);
		simulator->Run(nextThread, false);
    }
}

void Thread::Ssleep (bool finishing){

    Thread *nextThread;

	while ((nextThread = simulator->FindNextToRun()) == NULL){}

    // returns when it's time for us to run
    simulator->Run(nextThread, finishing); 
}

void Thread::StackAllocate(VoidFunctionPtr func, void *arg){

    stack = (int *) new char[StackSize * sizeof(int)];

    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
	stackTop --;
    *stack = STACK_FENCEPOST;

    machineState[PCState] = (void*)ThreadRoot;		//7 pc
    machineState[InitialPCState] = (void*)func;		//5	esi
    machineState[InitialArgState] = (void*)arg;		//3 edi

}


extern "C" {


	//Func�̱߳��溯����ַ
	//IniArg���溯��������ַ
	void* IniArg = (void*)0;
	void* Func = (void*)0;

	void ThreadRoot(){
		simulator->currentThread->Begin();
		__asm{	
			push   IniArg       /* �̺߳���func�Ĳ�����ջ */
			call   Func          /* call�̺߳���func */
			add    esp,4        /* �ͷŲ������ݵ�ջ�ռ� */
		}
		simulator->currentThread->Finish();
	}

	unsigned long _eax_save = 0;                          //ȫ���м����

	void SWITCH(Thread *oldThread, Thread *newThread){

		__asm{
			//align  2
			pop         edi        /* �ָ�edi */
			pop         esi        /* �ָ�esi */
			pop         ebx        /* �ָ�ebx */
			mov         esp,ebp    /* �ͷ�Ҫ�����ֲ������ռ� */
			pop         ebp        /* �ָ�ebp */

			mov    _eax_save,eax   /* �ݴ�eax, ע��:_eax_saveΪȫ�ֱ��� */

			mov    eax, [esp+4]    /* eax ָ��oldThread */
			mov    [_EBX+eax],ebx  /* ������ؼĴ���ֵ, ��oldThread�Ŀռ��� */
			mov    [_ECX+eax],ecx
			mov    [_EDX+eax],edx
			mov    [_ESI+eax],esi
			mov    [_EDI+eax],edi
			mov    [_EBP+eax],ebp
			mov    [_ESP+eax],esp  /* ����ջָ�� */

			mov     ebx,_eax_save  /* ȡ�ݴ��eax����ȫ�ֱ��� _eax_save�� */
			mov    [_EAX+eax],ebx  /* �����ʼeax��ֵ */
			mov    ebx,[esp+0]     /* ȡ���ص�ַ */
			mov    [_PC +eax],ebx  /* ���淵�ص�ַ */

			mov    eax,[esp+8]     /* eaxָ��newThread */
			mov    ebx,[_EAX+eax]  /* ȡnewThread�����eaxֵ*/
			mov    _eax_save,ebx   /* �ݴ浽 _eax_save */

			mov    ebx,[_EBX+eax]  /* �ָ�newThread����ļĴ���ֵ */
			mov    ecx,[_ECX+eax]
			mov    edx,[_EDX+eax]
			mov    IniArg,edx      //
			mov    esi,[_ESI+eax]
			mov    Func,esi        //
			mov    edi,[_EDI+eax]
			mov    ebp,[_EBP+eax]
			mov    esp,[_ESP+eax]  //�ָ�ջָ��

			mov    eax,[_PC +eax]  //���淵�ص�ַ��eax
			mov    [esp+0],eax     

			mov    eax,[_eax_save]

			ret                    /*ֱ�ӷ���*/
		}
	}

}