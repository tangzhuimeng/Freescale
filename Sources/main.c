#include <hidef.h>          /* common defines and macros */
#include "derivative.h"     /* derivative-specific definitions */
#include <MC9S12XS128.h>


                                                                    

#define ROW     40          //����ͼ��ɼ�������40��
#define COLUMN  75         //����ͼ��ɼ�������75��
#define CENTER  37         //��������

unsigned char Image_Data[ROW][COLUMN];    //ͼ������
unsigned char Line_C=0;          //����ͳ��ֵ
unsigned char l,r;
unsigned char m = 0;
unsigned char t=0;
unsigned char  Interval;        //�ɼ���Ч�������


unsigned char  THRESHOLD=80;  //�ڰ׶�ֵ��ͼ����ֵ(����ֵ���滷����任���仯)


unsigned char ctr_tiaobian [6]={37,37,37,37,37,37};            //���к���
unsigned char tiaobian_L[6];                         //�洢����������
unsigned char tiaobian_R[6];                         //�洢����������
int caiji[6]={14,15,16,19,20,21};                    //�ɼ���



unsigned char ctr=37; //����
                                          
  float K=32 ;   //������ϵ����
  int Y=40;      //�����١�
  float Error ;                                                                    
  float Last_Error=0 ;                                                               
  float Pre_Error=0 ;                                                               
  float This_Output;   
  float Last_Output=0;                                           
  float Learn_P=10 ;
  float Learn_D=1 ;
  float tempWeight_P=1;
  float tempWeight_D=1;
/***************************************************
** ��������: PLL_Init
** ��������: ʱ�ӳ�ʼ������
** ˵��:     ����ʱ��ѡ��40M
****************************************************/
void PLL_Init(void)
{
  CLKSEL=0x00; //40mhz
  SYNR =0xc0 | 0x04;                        
  REFDV=0x80 | 0x01; 
  PLLCTL_PLLON=1;
  POSTDIV=0X00;
  asm(nop);
  asm(nop);
  while(0==CRGFLG_LOCK); //���໷����
  CLKSEL_PLLSEL=1; //ѡ��PLLʱ��
}

/***************************************************
** ��������: TIM_Init
** ��������: �г��жϳ�ʼ������
** ˵��:     ���ж������ش���  ���ж��½��ش���
****************************************************/
void TIM_Init(void) 
{
TIOS =0x00;        //��ʱ��ͨ��0��1 Ϊ���벶׽
TSCR1=0x80;        //��ʱ��ʹ��
TCTL4=0x09;        //ͨ��0 ��׽������ͨ��1 ��׽�½���      3 2 1 0 ͨ��
TIE=0x03;          //ͨ��0��1 �ж�ʹ��
TFLG1=0xFF;        //���жϱ�־λ
}
 
/***************************************************
** ��������: PWM_Init
** ��������: ������Ƴ�ʼ������
** ˵��:     Ƶ��50HZ  
****************************************************/ 
 
 void PWM_Init()   // 4,5����
{     
    PWME=0x00;        // 00000000   disable PWM45
    PWMCAE=0; //���뷽ʽ,��
    PWMCLK=0xff;    //ѡ��A��Bʱ��ͨ��
    PWMPOL=0xff; //����,
    PWMPRCLK=0x00;    // clkA=clkB=40M
    PWMSCLA=20;        // clkSA=A/(2*20)=1M=0.001ms
    PWMSCLB=20;
    PWMCTL_CON23= 1;
    PWMCTL_CON45=1;   // connect channel 4,5 
    PWMCTL_CON67= 1;
    PWMPOL_PPOL3=0;
    PWMPOL_PPOL5=1;   // output low level after high level   duty=high level
    PWMPOL_PPOL7=0;    
    PWMCAE_CAE5=0;    // left aligned
    PWMPER23=200;
    PWMPER45=20000;   // period=PWMPER*SA=20000*0.001ms=20ms   frequency=1/period=50Hz
    PWMPER67=200;
    PWMDTY45=1500;       // pwm_duty=PWMDTY/PWMPER
    PWME=0x30;        // 00001010   enable PWM45
}



/************************************************** 
** ��������: ��ֵ��+��̬��ֵ+�˲�
** ��������: ͼ��Ԥ����
***************************************************/

void erzhi() 
{
  unsigned char i,j;
  unsigned int y=0;
 for(i = 0;i< ROW;i++) 
  {
    for(j=0;j<COLUMN;j++) 
    {y=y+Image_Data[i][j];
    }
    y=y/COLUMN-7;
    for(j = 0;j < COLUMN ;j++) 
    { 
    
          if(Image_Data[i][j]>y)  
                Image_Data[i][j]=1;        //����
                 else  Image_Data[i][j]=0;    //����
    }
    y=0;
  }
} 



/************************************************** 
** ��������: �ɼ�����
** ��������: 
***************************************************/
void tiaobian() // ������ɼ���
 { 
   int i; 
   int n;  //�������n����
   int a;  //�����ڵĺ���
   int b;   //�����ڼ���
   
   for(n=0;n<6;n++)         // 6��ѭ��
   {
	   b=caiji[n];            // ��Ӧ���ڵ���
	   a=ctr_tiaobian[n];     // ��Ӧ�����ĺ���
	   
	   for(i=a;i<75-2;i++)   
	   {
		    if((Image_Data[b][i]==1)&&(Image_Data[b][i-1]==1)&&(Image_Data[b][i+1]==0)&&(Image_Data[b][i+2]==0))    //��19�� �װ׺� �м��Ϊ�ұ���
			{
              tiaobian_R[n]=i+1; break;                                                                     //�����ұ�������r
			} 
          else tiaobian_R[n]=COLUMN;                                                                        //�����ұ������м�
	   }
       for(i=a;i>2;i--)                                                                 //��ctr��ʼ����Ѱ�ң���ʼ��Ϊ��Ļ�м�
	   {
	        if((Image_Data[b][i]==1)&&(Image_Data[b][i+1]==1)&&(Image_Data[b][i-1]==0)&&(Image_Data[b][i-2]==0))    //��19�� �ڰװ� �м��Ϊ�����
			{
              tiaobian_L[n]=i+1; break;                                                                      //�������������l
			}  
           else tiaobian_L[n]=0;                                                                         //������������м�
	   }
	   ctr_tiaobian[n]=(tiaobian_L[n]+tiaobian_R[n])/2;

   }
}

/************************************************** 
** ��������: ƫ����㺯�� ��ƫ��С��0Ӧ����ת��ƫ�������Ӧ���ҹգ�
** ��������: ��ÿ�а׵��������������������ƫ��
***************************************************/
 void piancha()  //��ƫ�
 {
	 if((tiaobian_R[3]=COLUMN)&&(tiaobian_L[3]=0)&&(tiaobian_R[4]=COLUMN)&&(tiaobian_L[4]=0))	 
	 ctr=(ctr_tiaobian[0]+ctr_tiaobian[1]+ctr_tiaobian[2])/4;	 
	 else	 
	 ctr=(ctr_tiaobian[3]+ctr_tiaobian[4]+ctr_tiaobian[5])/3;
 }
      
           
 /************************************************** 
** ��������: ���
** ��������: 
***************************************************/
void duoji()
{
	int a,b;                                                                                                                                                
                 
    float Weight_P;                                               
    float Weight_D;                                                                                                        
                       
    float X_P;
    float X_D;

	Error=CENTER-ctr;                         //����ƫ�

	if(Error<3&&Error>-3)       //��ƫ��Сʱ��
	{   
			PWMDTY45=1460;
      Last_Output=0;
	}
	else                                      //��ƫ���ʱ��
	{
    X_P=Error - Last_Error;
		X_D=Error - 2*Last_Error + Pre_Error;

		if(Last_Error<3&&Last_Error>-3) Last_Output=0;  //������ϸ�ƫ��С��ƫ�����¼��㡿
        //���ϴ���ƫ�������Ȩ�ء�
		if(Last_Error>0)                //���ϴ���ƫ�����Ȩ�ء�          
		{
		tempWeight_P = tempWeight_P + Learn_P*This_Output*Error*(2*Error-Last_Error);      //��������Ȩ�ء�P��
        tempWeight_D = tempWeight_D + Learn_D*This_Output*Error*(2*Error-Last_Error);      //��������Ȩ�ء�D��
		}
		else if(Last_Error<0)                //���ϴθ�ƫ�����Ȩ�ء� 
		{
		tempWeight_P = tempWeight_P + Learn_P*This_Output*Error*(Last_Error-2*Error);      //��������Ȩ�ء�P��
        tempWeight_D = tempWeight_D + Learn_D*This_Output*Error*(Last_Error-2*Error);      //��������Ȩ�ء�D��		
		}

        Weight_P= tempWeight_P /(tempWeight_P+tempWeight_D) ;
        Weight_D= tempWeight_D /(tempWeight_P+tempWeight_D) ;

		This_Output = Last_Output+ K*(Weight_P*X_P+Weight_D*X_D);

        b=(int)This_Output;   
        a=b+1460 ;
        if(a>1815)          //���ת�������350
        a=1815;   
        else 
        if(a<1135)          //���ת�������300
        a=1135;
        PWMDTY45=a;
       
		Last_Output=This_Output;
	}

	Pre_Error=Last_Error;
    Last_Error=Error;

}




/***************************************************
** ��������: ���
** ��������: 
** ˵��: 
****************************************************/  
void motor_f()  //�������
{
  PWME_PWME3=1;
  PWME_PWME7=0;
  PWMDTY23=200-Y;                    //�ٶ�
}


/***************************************************
** ��������: main
** ��������: ������
** ˵��: 
****************************************************/  
void main(void)
{
  DisableInterrupts;
  DDRA = 0X00;
  DDRB = 0xFF;
  PORTB= 0xFF; 
  PLL_Init();
    
  TIM_Init();
  PWM_Init() ;
  motor_f();      //�������
  EnableInterrupts;
  for(;;)
  { 
    if(t==40) 
    {
      t=0;
      TIE=0x00; 
   //////////////////////////////////////////////////
     
   
    erzhi();     //����ֵ����
  
	tiaobian();  //������ɼ���
 piancha();   //��ƫ�
   
	 duoji();     //�������
   
    /////////////////////////////////////////////////
      TIE=0x03; 
	 
    }
  } 
}

//---------------------�ж϶���---------------------
#pragma CODE_SEG NON_BANKED

/**************************************************       
** ��������: �жϴ�����
** ��������: ���жϴ�����
** ��    ��: �� 
** ��    ��: �� 
** ˵����  
***************************************************/ 
interrupt 8 void HREF_Count(void) 
{
  TFLG1_C0F = 1;
  m++;
  if ( m<6 || m>240 )       
  {
    return;//�ж��Ƿ���µ�һ����ʼ
  } 

  
  Interval=6;
  if(m%Interval==0)
  {
   t++;
    
Image_Data[Line_C][0] = PORTA; 
  asm(nop);
Image_Data[Line_C][1] = PORTA;
  asm(nop);
Image_Data[Line_C][2] = PORTA;
  asm(nop);
Image_Data[Line_C][3] = PORTA;
  asm(nop);
Image_Data[Line_C][4] = PORTA;
  asm(nop);
Image_Data[Line_C][5] = PORTA;
  asm(nop);
Image_Data[Line_C][6] = PORTA;
  asm(nop);
Image_Data[Line_C][7] = PORTA;
  asm(nop);
Image_Data[Line_C][8] = PORTA;
  asm(nop);
Image_Data[Line_C][9] = PORTA;
  asm(nop);
Image_Data[Line_C][10] = PORTA;
  asm(nop);
Image_Data[Line_C][11] = PORTA;
  asm(nop);
Image_Data[Line_C][12] = PORTA;
  asm(nop);
Image_Data[Line_C][13] = PORTA;
  asm(nop);
Image_Data[Line_C][14] = PORTA;
  asm(nop);
Image_Data[Line_C][15] = PORTA;
  asm(nop);
Image_Data[Line_C][16] = PORTA;
  asm(nop);
Image_Data[Line_C][17] = PORTA;
  asm(nop);
Image_Data[Line_C][18] = PORTA;
  asm(nop);
Image_Data[Line_C][19] = PORTA;
  asm(nop);
Image_Data[Line_C][20] = PORTA;
  asm(nop);
Image_Data[Line_C][21] = PORTA;
  asm(nop);
Image_Data[Line_C][22] = PORTA;
  asm(nop);
Image_Data[Line_C][23] = PORTA;
  asm(nop);
Image_Data[Line_C][24] = PORTA;
  asm(nop);
Image_Data[Line_C][25] = PORTA;
  asm(nop);
Image_Data[Line_C][26] = PORTA;
  asm(nop);
Image_Data[Line_C][27] = PORTA;
  asm(nop);
Image_Data[Line_C][28] = PORTA;
  asm(nop);
Image_Data[Line_C][29] = PORTA;
  asm(nop);
Image_Data[Line_C][30] = PORTA;
  asm(nop);
Image_Data[Line_C][31] = PORTA;
  asm(nop);
Image_Data[Line_C][32] = PORTA;
  asm(nop);
Image_Data[Line_C][33] = PORTA;
  asm(nop);
Image_Data[Line_C][34] = PORTA;
  asm(nop);
Image_Data[Line_C][35] = PORTA;
  asm(nop);
Image_Data[Line_C][36] = PORTA;
  asm(nop);
Image_Data[Line_C][37] = PORTA;
  asm(nop);
Image_Data[Line_C][38] = PORTA;
  asm(nop);
Image_Data[Line_C][39] = PORTA;
  asm(nop);
Image_Data[Line_C][40] = PORTA;
  asm(nop);
Image_Data[Line_C][41] = PORTA;
  asm(nop);
Image_Data[Line_C][42] = PORTA;
  asm(nop);
Image_Data[Line_C][43] = PORTA;
  asm(nop);
Image_Data[Line_C][44] = PORTA;
  asm(nop);
Image_Data[Line_C][45] = PORTA;
  asm(nop);
Image_Data[Line_C][46] = PORTA;
  asm(nop);
Image_Data[Line_C][47] = PORTA;
  asm(nop);
Image_Data[Line_C][48] = PORTA;
  asm(nop);
Image_Data[Line_C][49] = PORTA;
  asm(nop);
Image_Data[Line_C][50] = PORTA;
  asm(nop);
Image_Data[Line_C][51] = PORTA;
  asm(nop);
Image_Data[Line_C][52] = PORTA;
  asm(nop);
Image_Data[Line_C][53] = PORTA;
  asm(nop);
Image_Data[Line_C][54] = PORTA;
  asm(nop);
Image_Data[Line_C][55] = PORTA;
  asm(nop);
Image_Data[Line_C][56] = PORTA;
  asm(nop);
Image_Data[Line_C][57] = PORTA;
  asm(nop);
Image_Data[Line_C][58] = PORTA;
  asm(nop);
Image_Data[Line_C][59] = PORTA;
  asm(nop);
Image_Data[Line_C][60] = PORTA;
  asm(nop);
Image_Data[Line_C][61] = PORTA;
  asm(nop);
Image_Data[Line_C][62] = PORTA;
  asm(nop);
Image_Data[Line_C][63] = PORTA;
  asm(nop);
Image_Data[Line_C][64] = PORTA;
  asm(nop);
Image_Data[Line_C][65] = PORTA;
  asm(nop);
Image_Data[Line_C][66] = PORTA;
  asm(nop);
Image_Data[Line_C][67] = PORTA;
  asm(nop);
Image_Data[Line_C][68] = PORTA;
  asm(nop);
Image_Data[Line_C][69] = PORTA;
  asm(nop);
Image_Data[Line_C][70] = PORTA;
  asm(nop);
Image_Data[Line_C][71] = PORTA;
  asm(nop);
Image_Data[Line_C][72] = PORTA;
  asm(nop);
Image_Data[Line_C][73] = PORTA;
  asm(nop);
Image_Data[Line_C][74] = PORTA;
  asm(nop);

    
    Line_C++;
  }
  

}

/************************************************** 
** ��������: �жϴ�����
** ��������: ���жϴ�����
** ��    ��: �� 
** ��    ��: �� 
** ˵����  
***************************************************/
interrupt 9 void VSYN_Interrupt(void)
{
  TFLG1_C1F = 1; //�峡�ж�
  TFLG1_C0F = 1; //�����ж�

  Line_C = 0; //�м�����
  
    
  
}


#pragma CODE_SEG DEFAULT