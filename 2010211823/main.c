#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <setjmp.h>
#include <wiringPi.h>
#include <pthread.h>

#define SW_1 17
#define SW_2 27	
#define SW_3 22
#define LED_RED 11
#define LED_GREEN 13
#define LED_YELLOW 15
#define RAID0 0
#define RAID5 5
// Inotify 함수의 이벤트 처리를 위한 전처리 지정
#define Event ( sizeof (struct inotify_event) )
#define Buffer_size	(1024 * (Event + 64))

char buffer[Buffer_size];
// 명령어 수행 함수, RED LED Flash 함수, USB Input tracking 함수 선언
int Commend(const char *scriFileName, const char *usbName);
void *FlashLED(void *arg);
char *EventUSB();		// USB 저장장치 입력시 해당 이름을 반환

int main(int argc, char *argv) {
	int nBtnDelay = 0; // 초기화시 사용할 초기화 버튼 딜레이 변수
	int isSelectRaid = -1; // RAID 구성 여부 변수
	int succState; //RAID 구성 완료시 상태 저장 변수
	pthread_t p1; // 쓰레드 변수 선언
	// GPIO 셋업
	wiringPiSetupGpio();
	// pin 모드 설정
	pinMode(SW_1, INPUT);
	pinMode(SW_2, INPUT);
	pinMode(SW_3, INPUT);
	pinMode(LED_RED, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_YELLOW, OUTPUT);
    
	// 프로그램이 실행되면 3가지(빨간색, 초록색, 노란색) LED를 켜서 초기상태를 알린다
	digitalWrite(LED_RED, HIGH);
	digitalWrite(LED_GREEN, HIGH);
	digitalWrite(LED_YELLOW, HIGH);
	printf("=============================================================================\n");
	printf("====================== Wellcome Auto Backup System !! =======================\n");
	printf("=============================================================================\n");
	printf("                       < This is the initial state >                       \n\n");
	printf(" * button 1 -> RAID level 0 with Yellow LED\n * button 2 -> RAID level 5 with Green LED\n\n");
	printf(" # First step : Press button 1 or button 2 to select the desired RAID Level\n\n");
	printf("=============================================================================\n");
	printf("=============================================================================\n");
	// RAID 구성이 안될경우 루프
	while(isSelectRaid == -1)
	{
		// 첫번째 스위치
		if (digitalRead(SW_1) == 0 && digitalRead(SW_2) == 1)
		{
			// RAID Level 0
			digitalWrite(LED_RED, LOW);
			digitalWrite(LED_GREEN, LOW);
			digitalWrite(LED_YELLOW, LOW);
			// Commend() 함수에서 RAID 구성 명령 수행한 리턴값 반환
			succState = Commend("./raid0Script", "");
			// 성공하면 성공 문구 출력(Level 0)
			if(succState == 0) //If success RAID level 0 
			{
				digitalWrite(LED_YELLOW, HIGH);
				isSelectRaid = RAID0;
				printf("Button %d was pressed and RAID Level 0 setup complete\n", isSelectRaid);	
				printf("===========================================================\n");
				printf("===========================================================\n");
			}
					
		}
		// 두번째 스위치
		else if (digitalRead(SW_1) == 1 && digitalRead(SW_2) == 0)
		{
			// RAID Level 5
			digitalWrite(LED_RED, LOW);
			digitalWrite(LED_GREEN, LOW);
			digitalWrite(LED_YELLOW, LOW);
			// Commend() 함수에서 RAID 구성 명령 수행한 리턴값 반환
			succState = Commend("./raid5Script", "");
			// 성공하면 성공 문구 출력(Level 5)
			if(succState == 0) //If success RAID level 5
			{
				digitalWrite(LED_GREEN, HIGH);
				isSelectRaid = RAID5;
				printf("Button %d was pressed and RAID Level 5 setup complete\n", isSelectRaid);
				printf("=========================================================\n");
				printf("=========================================================\n");
			}
		}

		while(0 <= isSelectRaid) // Raid 선택이 되었을 때
		{
			printf("           < Backup is ready >         \n");
			printf(" * Current RAID Level is : %d\n", isSelectRaid);
			printf(" * Please insert USB                   \n");
			printf(" * Automatic incremental backups take  \n");
			printf("   place as soon as a USB is inserted  \n");
			printf("=======================================\n");
			printf("=======================================\n");
			// USB Tracking 함수 실행, USB가 입력되면 자동 마운트된 폴더의 경로를 가져옴
			// USB Input 대기
			char* abc = EventUSB();
			// 쓰레드 생성 -> RED LED Flash
			pthread_create(&p1, NULL, FlashLED, NULL);
			printf("Backup Start...\n\n");
			printf("Input USB name is : %s\n\n", abc);
				
			/* RAID 디스크가 마운트된 디렉토리와 USB 디스크가 마운트된 디렉토리를 
		     * 비교하여 백업을 수행한다(rsync 수행)
			 * RAID Level 0일때 RAID level 5일때 각각 알고리즘을 구성하는 포인트
			 */
			if(isSelectRaid == RAID0)
			{	
				// Commend() 함수에서 rsync 명령 수행(Level 0) 
				succState = Commend("./lev0BackupScript", abc);
				if(succState == 0)
					printf("Auto Incremental Backup Complete!!\n\n");
			}
			else if(isSelectRaid == RAID5)
			{
				// Commend() 함수에서 rsync 명령 수행(Level 5)
				succState = Commend("./lev5BackupScript", abc);
				if(succState == 0)
					printf("Auto Incremental backup Complete!!");
			}
			pthread_cancel(p1); // Backup 종료 후 쓰레드 종료
			digitalWrite(LED_RED, LOW);
			printf("====================================================\n");
			printf("                  Backup End                        \n");
			printf("====================================================\n");
			printf("====================================================\n");
			printf(" * Stay RAID Level?                                 \n");
			printf(" * Wait 10 seconds if you want to stay            \n\n");
			printf(" # If you want to reconfigure your RAID Level       \n");
			printf(" # Press Reset Button                               \n");
			printf("!!!!!!!!!!!!!!!!!!!!! Warrning !!!!!!!!!!!!!!!!!!!!!\n");
			printf(" ! If you press the Reset Button, all data          \n");
			printf(" ! on the current RAID Level %d volume\n", isSelectRaid);
			printf(" ! will be deleted                                  \n");
			printf("====================================================\n");
			printf("====================================================\n");
			
			nBtnDelay = 0;
			// 초기화 여부를 묻고 10초 동안 응답이 없으면 구성된 RAID Level로
			// 다시 USB Tracking
			while(++nBtnDelay < 9)
			{	
				//초기화 버튼이 눌리면 초기화
				if(digitalRead(SW_3) == 1)
				{
					isSelectRaid = -1;
					printf("       < Reset... RAID Level >              \n");
					printf(" * button 1 -> RAID level 0 with Yellow LED \n");
					printf(" * button 2 -> RAID level 5 with Green LED\n\n");
					printf(" # Press button 1 or button 2               \n");
					printf("   to select the desired RAID Level         \n");
					printf("============================================\n");
					printf("============================================\n");
					digitalWrite(LED_RED, HIGH);
					digitalWrite(LED_GREEN, HIGH);
					digitalWrite(LED_YELLOW, HIGH);
					break;
				}
				sleep(1);
			}
		
		}
 


	
	}
	return 0;
}

int Commend(const char* scriFileName, const char* usbName)
{
	int state;
	char *comm; // 합친 문자열 저장 포인터 변수
	FILE *fp = NULL;
	// 명령어 문자열을 담을 메모리 동적 할당
	comm = (char *)malloc(sizeof(char)*255);
	// 인자로 받은 문자열 합침
	sprintf(comm, "%s %s", scriFileName, usbName);
	memset(buffer, 0, Buffer_size); //버퍼 초기화
	//쉘 스크립트를 오픈하여 명령어 수행(popen() ->내부적 exec(), pipe))
	fp = popen(comm, "w");
	while(fgets(buffer, Buffer_size, fp) != NULL){
			printf("%s", buffer);
	}
	state = pclose(fp);
	free(comm);
	return state;
}
// RED LED Flash 함수
void *FlashLED(void *arg)
{   
    int i = 0;
    while (1)
    {
        if (i == 0)
        {
            digitalWrite(LED_RED, HIGH);
            i = 1;
        }
        else
        {
            digitalWrite(LED_RED, LOW);
            i = 0;
        }
        delay(100);
    }
}

char *EventUSB()
{
	char Fs_Path[4092]; // 감시할 파일 경로를 입력받을 변수
	strcpy(Fs_Path, "/media/pi");

	int ii; // 각 버퍼 사이즈 비교 대입 값을 위한 변수

	int Inotify_Buff; // inotify 함수를 대입할 고정 버퍼값을 위한 변수

	ii = inotify_init(); // inotify 모듈을 초기화 로드 한다
	// 타겟 파일의 존재를 파악하여 없다면 종료 시킨다
	if(access(Fs_Path, F_OK) == -1){
		fprintf(stderr, "Target %s does not exist... to EXIT.", Fs_Path);
		exit(0);
	}
	memset(buffer, 0, Buffer_size);
	// inotify 각 이벤트 발생에 따른 함수 실행을 위한 변수 지정
	Inotify_Buff = inotify_add_watch(ii, "/media/pi", IN_CREATE);
	// 지정한  타겟 파일에 대한 반복 감시 시작
	while(1){
		int origin_size, i = 0;
		origin_size = read(ii, buffer, Buffer_size);
		if(origin_size < 0){
			perror("read");
		}
		// 이벤트 발생을 위한 반복 수행
		while(i < origin_size){
			struct inotify_event *event = (struct inotify_event *) &buffer[i];
			// 타깃 디렉터리에서 파일이나 디렉토리가 생성 되었을 경우 이벤트 처리
			if(event->mask & IN_CREATE){
				return event->name;
			}
		}
	}
}
