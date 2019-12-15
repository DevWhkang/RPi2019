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
// Inotify �Լ��� �̺�Ʈ ó���� ���� ��ó�� ����
#define Event ( sizeof (struct inotify_event) )
#define Buffer_size	(1024 * (Event + 64))

char buffer[Buffer_size];
// ��ɾ� ���� �Լ�, RED LED Flash �Լ�, USB Input tracking �Լ� ����
int Commend(const char *scriFileName, const char *usbName);
void *FlashLED(void *arg);
char *EventUSB();		// USB ������ġ �Է½� �ش� �̸��� ��ȯ

int main(int argc, char *argv) {
	int nBtnDelay = 0; // �ʱ�ȭ�� ����� �ʱ�ȭ ��ư ������ ����
	int isSelectRaid = -1; // RAID ���� ���� ����
	int succState; //RAID ���� �Ϸ�� ���� ���� ����
	pthread_t p1; // ������ ���� ����
	// GPIO �¾�
	wiringPiSetupGpio();
	// pin ��� ����
	pinMode(SW_1, INPUT);
	pinMode(SW_2, INPUT);
	pinMode(SW_3, INPUT);
	pinMode(LED_RED, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_YELLOW, OUTPUT);
    
	// ���α׷��� ����Ǹ� 3����(������, �ʷϻ�, �����) LED�� �Ѽ� �ʱ���¸� �˸���
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
	// RAID ������ �ȵɰ�� ����
	while(isSelectRaid == -1)
	{
		// ù��° ����ġ
		if (digitalRead(SW_1) == 0 && digitalRead(SW_2) == 1)
		{
			// RAID Level 0
			digitalWrite(LED_RED, LOW);
			digitalWrite(LED_GREEN, LOW);
			digitalWrite(LED_YELLOW, LOW);
			// Commend() �Լ����� RAID ���� ��� ������ ���ϰ� ��ȯ
			succState = Commend("./raid0Script", "");
			// �����ϸ� ���� ���� ���(Level 0)
			if(succState == 0) //If success RAID level 0 
			{
				digitalWrite(LED_YELLOW, HIGH);
				isSelectRaid = RAID0;
				printf("Button %d was pressed and RAID Level 0 setup complete\n", isSelectRaid);	
				printf("===========================================================\n");
				printf("===========================================================\n");
			}
					
		}
		// �ι�° ����ġ
		else if (digitalRead(SW_1) == 1 && digitalRead(SW_2) == 0)
		{
			// RAID Level 5
			digitalWrite(LED_RED, LOW);
			digitalWrite(LED_GREEN, LOW);
			digitalWrite(LED_YELLOW, LOW);
			// Commend() �Լ����� RAID ���� ��� ������ ���ϰ� ��ȯ
			succState = Commend("./raid5Script", "");
			// �����ϸ� ���� ���� ���(Level 5)
			if(succState == 0) //If success RAID level 5
			{
				digitalWrite(LED_GREEN, HIGH);
				isSelectRaid = RAID5;
				printf("Button %d was pressed and RAID Level 5 setup complete\n", isSelectRaid);
				printf("=========================================================\n");
				printf("=========================================================\n");
			}
		}

		while(0 <= isSelectRaid) // Raid ������ �Ǿ��� ��
		{
			printf("           < Backup is ready >         \n");
			printf(" * Current RAID Level is : %d\n", isSelectRaid);
			printf(" * Please insert USB                   \n");
			printf(" * Automatic incremental backups take  \n");
			printf("   place as soon as a USB is inserted  \n");
			printf("=======================================\n");
			printf("=======================================\n");
			// USB Tracking �Լ� ����, USB�� �ԷµǸ� �ڵ� ����Ʈ�� ������ ��θ� ������
			// USB Input ���
			char* abc = EventUSB();
			// ������ ���� -> RED LED Flash
			pthread_create(&p1, NULL, FlashLED, NULL);
			printf("Backup Start...\n\n");
			printf("Input USB name is : %s\n\n", abc);
				
			/* RAID ��ũ�� ����Ʈ�� ���丮�� USB ��ũ�� ����Ʈ�� ���丮�� 
		     * ���Ͽ� ����� �����Ѵ�(rsync ����)
			 * RAID Level 0�϶� RAID level 5�϶� ���� �˰����� �����ϴ� ����Ʈ
			 */
			if(isSelectRaid == RAID0)
			{	
				// Commend() �Լ����� rsync ��� ����(Level 0) 
				succState = Commend("./lev0BackupScript", abc);
				if(succState == 0)
					printf("Auto Incremental Backup Complete!!\n\n");
			}
			else if(isSelectRaid == RAID5)
			{
				// Commend() �Լ����� rsync ��� ����(Level 5)
				succState = Commend("./lev5BackupScript", abc);
				if(succState == 0)
					printf("Auto Incremental backup Complete!!");
			}
			pthread_cancel(p1); // Backup ���� �� ������ ����
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
			// �ʱ�ȭ ���θ� ���� 10�� ���� ������ ������ ������ RAID Level��
			// �ٽ� USB Tracking
			while(++nBtnDelay < 9)
			{	
				//�ʱ�ȭ ��ư�� ������ �ʱ�ȭ
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
	char *comm; // ��ģ ���ڿ� ���� ������ ����
	FILE *fp = NULL;
	// ��ɾ� ���ڿ��� ���� �޸� ���� �Ҵ�
	comm = (char *)malloc(sizeof(char)*255);
	// ���ڷ� ���� ���ڿ� ��ħ
	sprintf(comm, "%s %s", scriFileName, usbName);
	memset(buffer, 0, Buffer_size); //���� �ʱ�ȭ
	//�� ��ũ��Ʈ�� �����Ͽ� ��ɾ� ����(popen() ->������ exec(), pipe))
	fp = popen(comm, "w");
	while(fgets(buffer, Buffer_size, fp) != NULL){
			printf("%s", buffer);
	}
	state = pclose(fp);
	free(comm);
	return state;
}
// RED LED Flash �Լ�
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
	char Fs_Path[4092]; // ������ ���� ��θ� �Է¹��� ����
	strcpy(Fs_Path, "/media/pi");

	int ii; // �� ���� ������ �� ���� ���� ���� ����

	int Inotify_Buff; // inotify �Լ��� ������ ���� ���۰��� ���� ����

	ii = inotify_init(); // inotify ����� �ʱ�ȭ �ε� �Ѵ�
	// Ÿ�� ������ ���縦 �ľ��Ͽ� ���ٸ� ���� ��Ų��
	if(access(Fs_Path, F_OK) == -1){
		fprintf(stderr, "Target %s does not exist... to EXIT.", Fs_Path);
		exit(0);
	}
	memset(buffer, 0, Buffer_size);
	// inotify �� �̺�Ʈ �߻��� ���� �Լ� ������ ���� ���� ����
	Inotify_Buff = inotify_add_watch(ii, "/media/pi", IN_CREATE);
	// ������  Ÿ�� ���Ͽ� ���� �ݺ� ���� ����
	while(1){
		int origin_size, i = 0;
		origin_size = read(ii, buffer, Buffer_size);
		if(origin_size < 0){
			perror("read");
		}
		// �̺�Ʈ �߻��� ���� �ݺ� ����
		while(i < origin_size){
			struct inotify_event *event = (struct inotify_event *) &buffer[i];
			// Ÿ�� ���͸����� �����̳� ���丮�� ���� �Ǿ��� ��� �̺�Ʈ ó��
			if(event->mask & IN_CREATE){
				return event->name;
			}
		}
	}
}
