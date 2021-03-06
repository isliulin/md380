//=====================================================================
//
// 性能与功能的接口
//
//=====================================================================


#include "f_funcCode.h"
#include "f_runSrc.h"
#include "f_main.h"
#include "f_frqSrc.h"
#include "f_io.h"
#include "f_menu.h"
#include "f_posCtrl.h"

#if F_DEBUG_RAM
#define DEBUG_F_INTERFACE                   0   // 性能, 功能交互
#define DEBUG_F_POWER_ADD                   0   // 累计耗电量
#elif 1
#define DEBUG_F_INTERFACE                   1   // 性能, 功能交互
#define DEBUG_F_POWER_ADD                   1   // 累计耗电量
#endif

#define DROOP_VOLTAGE_LEVEL 6500  // 下垂控制母线电压基准
//========================================
// 功能传递给性能的数据
Uint16 frq2Core;
Uint16 frqCurAim2Core;
Uint16 rsvdData;            // 保留数据
Uint16 driveCoeffFrqFdb;
Uint16 vcSpdLoopKp1;
Uint16 vcSpdLoopKp2;
Uint16 jerkFf;

extern Uint16 vfSeparateVol;

Uint16 speedMotor;      // 
Uint16 motorRun;        // 编码器反馈速度

int32 frqRun;           // 性能实际发出的频率(对VF而言，是同步转速), 0.01Hz
int32 frqVFRun;           // 性能实际发出的频率(对VF而言，是同步转速), 0.01Hz
int32 frqVFRunRemainder;  // 计算余值
Uint16 outVoltage;
Uint16 outCurrent;      // Q12
Uint16 generatrixVoltage;   // 单位: 0.1V
int16 outPower;         // 单位为0.1KW
Uint16 errorCodeFromMotor;

Uint16 errorsCodeFromMotor[2];
Uint16 motorErrInfor[5];

//#define OUT_CURRENT_FILTER_TIME 30  // 输出电流滤波时间系数
//LOCALF LowPassFilter currentInLpf = LPF_DEFALUTS;

Uint16 outCurrentDispOld;
//Uint16 PGErrorFlag;     // 参数辨识后PG卡故障标志  2-编码器线数设定错误  1-未接编码器

// 性能传递给功能即时
int16 gPhiRt;         // 功率因数角度
int16 gPhiRtDisp;     // 显示值
Uint16 pmsmRotorPos;  // 同步机转子位置(性能实时更新)
Uint16 frqFdb;        // 电机反馈转速
Uint16 frqFdbDisp;    // 电机反馈速度显示值
int32  frqFdbTmp;
Uint16 frqFdbFlag;
Uint16 torqueCurrent;
Uint16 enCoderPosition;
Uint16 ABZPos;
Uint16 ZCount;        // Z信号计数
Uint16 antiVibrateCoeff;

#if DEBUG_F_INTERFACE
void copyDataFunc2Motor2ms(void);
// 性能传递给功能的数据
Uint16 currentOc;
Uint16 currentPu;
Uint16 spdLoopOut;                  // 性能部分的速度环输出


LOCALF Uint32 softOCTicker;         // 软件过流计时
extern Uint16 softOcDoFlag;

// 功能==>性能的参数，0.5ms
void copyDataFunc2Motor05ms(void)
{
    gSendToMotor05MsDataBuff[0] = dspMainCmd.all;                            // 0   主命令字
    gSendToMotor05MsDataBuff[1] = dspMainCmd1.all;                           // 1   主命令字1
    gSendToMotor05MsDataBuff[2] = tuneCmd;                                   // 2   调谐选择
    gSendToMotor05MsDataBuff[3] = motorFc.motorPara.elem.motorType;          // 3   电机类型
    gSendToMotor05MsDataBuff[4] = frq2Core;                                  // 4   实时速度给定
    gSendToMotor05MsDataBuff[5] = vfSeparateVol;                             // 5   VF分离时的输出电压
    gSendToMotor05MsDataBuff[6] = motorFc.motorPara.elem.ratingVoltage;      // 6   电机额定电压
    gSendToMotor05MsDataBuff[7] = motorFc.motorPara.elem.ratingCurrent;      // 7   电机额定电流
    gSendToMotor05MsDataBuff[8] = motorFc.motorPara.elem.ratingFrq;          // 8   电机额定频率
    gSendToMotor05MsDataBuff[9] = funcCode.code.vfCurve;                     // 9   VF曲线选择
    gSendToMotor05MsDataBuff[10] = funcCode.code.ovGain;                      // 10  F9-03 过压失速增益
    gSendToMotor05MsDataBuff[11] = funcCode.code.ovPoint;                     // 11  F9-04 过压失速保护电压    
    gSendToMotor05MsDataBuff[12] = funcCode.code.ocGain;                      // 12  F9-05 过流失速增益
    gSendToMotor05MsDataBuff[13] = funcCode.code.ocPoint;                     // 13  F9-06 过流失速保护电流
}

// 功能==>性能的参数，2ms
// 调试参数，不放在这里，直接传递给gSendToMotor2MsDataBuff[]的对应位置
void copyDataFunc2Motor2ms(void)
{
#if  DEBUG_F_INTERFACE
    gSendToMotor2MsDataBuff[0] = dspSubCmd.all;                             // 0    辅助命令字
    gSendToMotor2MsDataBuff[1] = frqCurAim2Core;                            // 1    目标频率
    gSendToMotor2MsDataBuff[2] = funcCode.code.maxFrq;                      // 2    最大频率
    gSendToMotor2MsDataBuff[3] = funcCode.code.carrierFrq;                  // 3    载波频率
    gSendToMotor2MsDataBuff[4] = motorFc.motorPara.elem.ratingPower;        // 4    电机额定功率

#if 1    
    memcpy(&gSendToMotor2MsDataBuff[5], &motorFc.motorPara.elem.ratingSpeed, 6);
#else 
    gSendToMotor2MsDataBuff[5] = motorFc.motorPara.elem.ratingSpeed;        // 5    电机额定转速
    gSendToMotor2MsDataBuff[6] = motorFc.motorPara.elem.statorResistance;   // 6    定子电阻
    gSendToMotor2MsDataBuff[7] = motorFc.motorPara.elem.rotorResistance;    // 7    转子电阻
    gSendToMotor2MsDataBuff[8] = motorFc.motorPara.elem.leakInductance;     // 8    漏感
    gSendToMotor2MsDataBuff[9] = motorFc.motorPara.elem.mutualInductance;   // 9   互感
    gSendToMotor2MsDataBuff[10] = motorFc.motorPara.elem.zeroLoadCurrent ;  // 10   空载电流
#endif

#if 1    
    memcpy(&gSendToMotor2MsDataBuff[11], &motorFc.motorPara.elem.pmsmRs, 3);
#else 
    gSendToMotor2MsDataBuff[11] = motorFc.motorPara.elem.pmsmRs;             // 11   同步机定子电阻
    gSendToMotor2MsDataBuff[12] = motorFc.motorPara.elem.pmsmLd;             // 12   同步机D轴电感
    gSendToMotor2MsDataBuff[13] = motorFc.motorPara.elem.pmsmLq;             // 13   同步机Q轴电感
#endif

    gSendToMotor2MsDataBuff[14] = motorFc.vcPara.vcSpdLoopKp1;               // 14   速度环Kp1
    gSendToMotor2MsDataBuff[15] = motorFc.vcPara.vcSpdLoopTi1;               // 15   速度环Ti1
    gSendToMotor2MsDataBuff[16] = motorFc.vcPara.vcSpdLoopKp2;               // 16   速度环Kp2
    gSendToMotor2MsDataBuff[17] = motorFc.vcPara.vcSpdLoopTi2;               // 17   速度环Ti2
    gSendToMotor2MsDataBuff[18] = motorFc.vcPara.vcSpdLoopChgFrq1;           // 18   速度环切换频率1
    gSendToMotor2MsDataBuff[19] = motorFc.vcPara.vcSpdLoopChgFrq2;           // 19   速度环切换频率2
    gSendToMotor2MsDataBuff[20] = motorFc.vcPara.vcSlipCompCoef;             // 20   VC转差补偿增益    
    gSendToMotor2MsDataBuff[21] = motorFc.vcPara.vcSpdLoopFilterTime;        // 21   速度环调节器输出滤波时间
    gSendToMotor2MsDataBuff[22] = motorFc.pgPara.elem.encoderPulse;          // 22   编码器脉冲数
    gSendToMotor2MsDataBuff[23] = motorFc.vcPara.vcOverMagGain;              // 23   矢量控制过励磁增益
    gSendToMotor2MsDataBuff[24] = funcCode.code.loseLoadLevel;               // 24   掉载检测水平
    gSendToMotor2MsDataBuff[25] = funcCode.code.loseLoadTime;                // 25   掉载检测时间
    gSendToMotor2MsDataBuff[26] = upperTorque;                               // 26   转矩限定
    gSendToMotor2MsDataBuff[27] = motorFc.torqueBoost;                       // 27   转矩提升

#if 1    
    memcpy(&gSendToMotor2MsDataBuff[28], &funcCode.code.boostCloseFrq, 9);
#else 
    gSendToMotor2MsDataBuff[28] = funcCode.code.boostCloseFrq;               // 28   VF提升截止频率
    gSendToMotor2MsDataBuff[29] = funcCode.code.vfFrq1;                      // 29   VF频率点1
    gSendToMotor2MsDataBuff[30] = funcCode.code.vfVol1;                      // 30   VF电压点1
    gSendToMotor2MsDataBuff[31] = funcCode.code.vfFrq2;                      // 31   VF频率点2
    gSendToMotor2MsDataBuff[32] = funcCode.code.vfVol2;                      // 32   VF电压点2
    gSendToMotor2MsDataBuff[33] = funcCode.code.vfFrq3;                      // 33   VF频率点3
    gSendToMotor2MsDataBuff[34] = funcCode.code.vfVol3;                      // 34   VF电压点3
    gSendToMotor2MsDataBuff[35] = funcCode.code.slipCompCoef;                // 35   VF转差补偿
    gSendToMotor2MsDataBuff[36] = funcCode.code.vfOverMagGain;               // 36   VF过励磁增益
#endif
    
    gSendToMotor2MsDataBuff[37] = motorFc.antiVibrateGain;                   // 37   VF抑制振荡增益
    gSendToMotor2MsDataBuff[38] = funcCode.code.startBrakeCurrent;           // 38   启动直流制动电流
    gSendToMotor2MsDataBuff[39] = funcCode.code.stopBrakeCurrent;            // 39   停机直流制动电流
    gSendToMotor2MsDataBuff[40] = funcCode.code.brakeDutyRatio;              // 40   制动使用率
    gSendToMotor2MsDataBuff[41] = funcCode.code.overloadGain;                // 41   电机过载保护增益
    gSendToMotor2MsDataBuff[42] = funcCode.code.foreOverloadCoef;            // 42   电机过载预报警系数
    gSendToMotor2MsDataBuff[43] = funcCode.code.softPwm;                     // 43   随机PWM选择
    gSendToMotor2MsDataBuff[44] = funcCode.code.curSampleDelayComp;          // 44   电流检测延时补偿
    gSendToMotor2MsDataBuff[45] = funcCode.code.uvPoint;                     // 45   欠压点 
    gSendToMotor2MsDataBuff[46] = motorFc.pgPara.elem.pgType;                // 46   编码器类型
    gSendToMotor2MsDataBuff[47] = driveCoeffFrqFdb;                          // 47   测速传动比
    gSendToMotor2MsDataBuff[48] = funcCode.code.inverterType;                // 48   变频器机型
    gSendToMotor2MsDataBuff[49] = funcCode.code.inverterGpType;              // 49   GP机型
    gSendToMotor2MsDataBuff[50] = funcCode.code.tempCurve;                   // 50   温度曲线选择
    gSendToMotor2MsDataBuff[51] = funcCode.code.volJudgeCoeff;               // 51   电压校正系数
    gSendToMotor2MsDataBuff[52] = funcCode.code.curJudgeCoeff;               // 52   电流校正系数
    gSendToMotor2MsDataBuff[53] = funcCode.code.uvGainWarp;                  // 53   UV两相增益偏差
    gSendToMotor2MsDataBuff[54] = funcCode.code.svcMode;                     // 54   SVC优化选择 0-不优化  1-优化模式1  2-优化模式2
    gSendToMotor2MsDataBuff[55] = funcCode.code.deadTimeSet;                 // 55   死区时间调整-1140V专用
    gSendToMotor2MsDataBuff[56] = funcCode.code.speedTrackMode;              // 56   转速跟踪
    gSendToMotor2MsDataBuff[57] = funcCode.code.speedTrackVelocity;          // 57   转速跟踪
    gSendToMotor2MsDataBuff[58] = funcCode.code.pmsmRotorPos;                // 58   同步机转子位置
    gSendToMotor2MsDataBuff[59] = motorFc.pgPara.elem.enCoderPole;           // 59   旋变极对数
    gSendToMotor2MsDataBuff[60] = motorFc.motorPara.elem.pmsmCoeff;          // 60   同步机反电动势系数  

#if 1    
    memcpy(&gSendToMotor2MsDataBuff[61], &motorFc.vcPara.mAcrKp, 4);
#else     
    gSendToMotor2MsDataBuff[61] = motorFc.vcPara.mAcrKp;                     // 61  M轴电流环Kp
    gSendToMotor2MsDataBuff[62] = motorFc.vcPara.mAcrKi;                     // 62  M轴电流环Ki
    gSendToMotor2MsDataBuff[63] = motorFc.vcPara.tAcrKp;                     // 63  T轴电流环Kp
    gSendToMotor2MsDataBuff[64] = motorFc.vcPara.tAcrKi;                     // 64  T轴电流环Ki
#endif

    gSendToMotor2MsDataBuff[65] = motorFc.pgPara.elem.enCoderDir;            // 65  旋变方向
    gSendToMotor2MsDataBuff[66] = motorFc.pgPara.elem.enCoderAngle;          // 66  编码器安装角
    gSendToMotor2MsDataBuff[67] = funcCode.code.pwmMode;                     // 67 DPWM切换上限频率
    gSendToMotor2MsDataBuff[68] = motorFc.pgPara.elem.uvwSignDir;            // 68 UVW信号方向
    gSendToMotor2MsDataBuff[69] = motorFc.pgPara.elem.uvwSignAngle;          // 69 UVW信号零点位置角

#if 1    
    memcpy(&gSendToMotor2MsDataBuff[70], &motorFc.vcPara.weakFlusMode, 5);
#else  
    gSendToMotor2MsDataBuff[71] = motorFc.vcPara.weakFlusMode;               // 70 F2-18 同步机弱磁模式
    gSendToMotor2MsDataBuff[72] = motorFc.vcPara.weakFlusCoef;               // 71 F2-19 同步机弱磁系数
    gSendToMotor2MsDataBuff[73] = motorFc.vcPara.weakFlusCurMax;             // 72 F2-20 最大弱磁电流
    gSendToMotor2MsDataBuff[74] = motorFc.vcPara.weakFlusAutoCoef;           // 73 F2-21 弱磁自动调谐系数
    gSendToMotor2MsDataBuff[75] = motorFc.vcPara.weakFlusIntegrMul;          // 74 F2-22 弱磁积分倍数
#endif  

#endif
}

// 功能==>性能的调试参数，2ms
// 调试参数，不放在这里，直接传递给gSendToMotor2MsDataBuff[]的对应位置
void copyDataFunc2CF2ms(void)
{
#if 1
    memcpy(&gSendToMotor2MsDataBuff[FUNC_TO_MOTOR_2MS_DATA_NUM], &funcCode.group.cf[0], 40);
#elif 1
    gSendToMotor2MsDataBuff[98] = funcCode.group.cf[0];
    ...
    gSendToMotor2MsDataBuff[137] = funcCode.group.cf[39];
    
#endif    
}                                                  
                                                   
// 调试参数，不放在这里，直接读gSendToFunctionDataBuff[]的对应位置
extern Uint16 errorInfoFromMotor;     
extern Uint16 motorCtrlTuneStatus;

void copyDataCore2Func2ms(void)
{
    dspStatus.all = gSendToFunctionDataBuff[0];                             // 0    状态字
    motorCtrlTuneStatus = gSendToFunctionDataBuff[1];                       // 1    参数辨识状态字
    errorsCodeFromMotor[0] = gSendToFunctionDataBuff[2];                    // 2    性能故障代码1
    errorsCodeFromMotor[1] = gSendToFunctionDataBuff[3];                    // 3    性能故障代码2
    motorErrInfor[0] = gSendToFunctionDataBuff[4];                      // 4    故障提示信息1
    motorErrInfor[1] = gSendToFunctionDataBuff[5];                      // 5    故障提示信息2
    motorErrInfor[2] = gSendToFunctionDataBuff[6];                      // 6    故障提示信息3
    motorErrInfor[3] = gSendToFunctionDataBuff[7];                      // 7    故障提示信息4
    motorErrInfor[4] = gSendToFunctionDataBuff[8];                      // 8    故障提示信息5
    currentOc = gSendToFunctionDataBuff[9];                                 // 9    过流时的电流
    speedMotor = gSendToFunctionDataBuff[10];                                // 10   输出频率
    outVoltage = gSendToFunctionDataBuff[11];                                // 11   输出电压
    funcCode.code.radiatorTemp = gSendToFunctionDataBuff[12];                // 12   散热器温度
    currentPu = gSendToFunctionDataBuff[13];                                 // 13   电流标幺值，当电机额定电流与变频器的额定电流相差较大时，可能与电机额定电流不同.
    funcCode.code.motorSoftVersion = gSendToFunctionDataBuff[14];            // 14   DSP软件版本号，放入FF-07
    aiDeal[0].sample = gSendToFunctionDataBuff[15];                          // 15   AI1的采样值，已经滤波
    aiDeal[1].sample = gSendToFunctionDataBuff[16];                          // 16   AI2的采样值，已经滤波
    aiDeal[2].sample = gSendToFunctionDataBuff[17];                          // 17   AI3的采样值，已经滤波
    generatrixVoltage = gSendToFunctionDataBuff[18];                         // 18   直流母线电压
    torqueCurrent = gSendToFunctionDataBuff[19];                             // 19   转矩电流，基值是传递过来的“实际使用的电流基值”
    outCurrent = gSendToFunctionDataBuff[20];                                // 20   输出电流
    pmsmRotorPos = gSendToFunctionDataBuff[21];                              // 21   同步机转子位置
    //PGErrorFlag = gSendToFunctionDataBuff[22];                               // 22   参数辨识PG卡故障标志
    motorRun = gSendToFunctionDataBuff[23];                                  // 23   电机反馈频率  
    outPower = gSendToFunctionDataBuff[24];                                  // 24   输出功率
    itDisp   = gSendToFunctionDataBuff[25];                                  // 25   输出转矩
    enCoderPosition = gSendToFunctionDataBuff[26];                           // 26   旋变机械位置
    gPhiRt = gSendToFunctionDataBuff[27];                                    // 27   功率因数角
    ZCount = gSendToFunctionDataBuff[28];                                    // 28   Z信号计数器
    antiVibrateCoeff = gSendToFunctionDataBuff[29];                          // 29   VF振荡系数
}

// 性能到功能监视参数
void copyDataCore2Uf2ms(void)
{
#if 1
    memcpy(&funcCode.group.uf[0], &gSendToFunctionDataBuff[MOTOR_TO_Func_2MS_DATA_NUM], 30);
#elif 1
    funcCode.group.uf[0]  = gSendToFunctionDataBuff[26];
...
    funcCode.group.uf[29] = gSendToFunctionDataBuff[55];
    
#endif    
}

void copyDataCore2Func05ms(void)
{
}


#elif 1

Uint16 currentPu;

#endif



void UpdateDataCore2Func(void);
void UpdateDataFunc2Core0p5ms(void);
void UpdateDataFunc2Core2ms(void);


#define CHECK_SPEED_DIR_NO     0   // 未检测编码器反馈速度方向
#define CHECK_SPEED_DIR_START  1   // 开始编码器反馈速度方向检�
#define CHECK_SPEED_DIR_END    2   // 完成编码器反馈速度方向检测
#define CHECK_SPEED_DIR_QUITE  3   // 退出编码器反馈速度方向检测
//=====================================================================
//
// 检测编码器反馈速度与实际转速对应
// 同向:编码器反馈方向即为电机方向
// 反向:编码器反馈方向取反后为电机方向
//
//=====================================================================
void checkSpeedFdbDir(int32 frqFdb)
{   
    
    static Uint16 checkFlag = CHECK_SPEED_DIR_NO;  // 反馈与速度方向检测状态
    static Uint16 speedDir;
    static Uint16 speedDirBak = 0;
    static Uint16 speedFdbDirTcnt = 0;
    
    if (checkFlag == CHECK_SPEED_DIR_QUITE)
    {
        return;
    }
    
    // 停机且反馈速度为0时启动方向检测标志
    if ((!runFlag.bit.run)
        && (!frqFdb)
        && (checkFlag == CHECK_SPEED_DIR_NO)
        )
    {
        checkFlag = CHECK_SPEED_DIR_START;         // 开始检测
    }

    if (runFlag.bit.run                                // 处于运行中
        &&(checkFlag == CHECK_SPEED_DIR_START)         // 之前无判断方向
        )
    {
        speedDirBak = speedDir;  // 备份前一个方向
        
        // 实际运行频率与反馈频率方向一致
        if ((int64)frqTmp*((int32)frqFdb) > 0) 
        {
            speedDir = 0;  // 编码器反馈与运行频率同向
        }
        // 实际运行频率与反馈频率方向相反
        else if ((int64)frqTmp*((int32)frqFdb) < 0) 
        {
            speedDir = 1;  // 编码器反馈与运行频率反向
        }

        if (speedDir == speedDirBak)
        {
            speedFdbDirTcnt++;
        }
        else
        {
            speedFdbDirTcnt = 0;
        }
        
        // 完成编码器反馈速度方向检测
        if (speedFdbDirTcnt > 2500)      // 5s内检测的为一个方向标志时判断该值为真实方向
        {
            checkFlag = CHECK_SPEED_DIR_END;
        }
    }

    if ((!runFlag.bit.run)
        &&(checkFlag == CHECK_SPEED_DIR_END)
        )
    {
        funcCode.code.speedFdbDir = speedDir;
        checkFlag = CHECK_SPEED_DIR_QUITE;    // 退出检测
    }
}

//=====================================================================
//
// 更新性能传递给功能的交互数据
//
//=====================================================================
Uint16 powerAddDec;

#define POWER_ADD_COUNT_DEC    5000
#define POWER_ADD_COUNT_INT    3600 // 100*3600*500/5000
void UpdateDataCore2Func(void)
{
    int32 tmp;
    int32 frqFdbDispTmp;
    static int32 outCurrentDispOldTmp;
    static LowPassFilter frqFdbLpf = LPF_DEFALUTS;

#if DEBUG_F_INTERFACE
    copyDataCore2Func2ms();     // 性能传递给功能2ms
    copyDataCore2Func05ms();    // 性能传递给功能0.5ms
   
#endif


#if F_DEBUG_RAM    //+= 没有性能程序时候的调试
    if (ERROR_DEALING)        // 功能已经处理，性能将故障码清零
    {
        errorCodeFromMotor = ERROR_NONE;     // errorCodeFromMotor = 0
    }

    if (dspMainCmd.bit.run && (!errorCodeFromMotor)) // 功能有运行命令，且性能没有错误
    {
        dspStatus.bit.run = 1;
    }
    else
    {
        dspStatus.bit.run = 1;
    }

    speedMotor = frq2Core;

    {
        static int16 k;
        
        if (++k >= (Uint16)(200/RUN_CTRL_PERIOD))  // _ms之后，才将这些标志置为1
        {
            dspStatus.bit.runEnable = 1;
        }
    }

    currentPu = funcCode.code.motorParaM1.elem.ratingCurrent;  // currentPu

    speedMotor = frq2Core;  // 调试时，反馈速度总是 给定速度

    generatrixVoltage = 12345;      // 母线电压
    outCurrent = 2048;              // 输出电流, Q12
    outVoltage = 1<<12;             // 输出电压, Q12
//    radiatorTemp = -9;              // 散热器温度
#endif


#if DEBUG_F_MOTOR_FUNCCODE
#if !F_DEBUG_RAM
// 显示，调试使用，motor2func
    copyDataCore2Uf2ms();  // 性能传递给功能，显示
#endif
#endif

    // 变频器实际运行频率
    frqRun = ((int32)(int16)speedMotor * (int32)frqPuQ15 + (1 << 14)) >> 15;

    if (motorFc.motorCtrlMode == FUNCCODE_motorCtrlMode_VF)
    {
        // 计算过压过流抑制时使用的频率
    	frqVFRun = ((int32)(int16)speedMotor * (int32)frqPuQ15) >> 15;


    	if (frqVFRun < frqRun)
    	{
    		frqVFRunRemainder = ((int32)(int16)speedMotor * (int32)frqPuQ15) - (frqRun<<15);
    	}
    	else
    	{
    	    // 计算余值
    	    frqVFRunRemainder = ((int32)(int16)speedMotor * (int32)frqPuQ15) - (frqVFRun<<15);
    	}
    }
    else

    {
        frqVFRunRemainder = 0;
    }

    // 电机实际转速,编码器测速
    frqFdbTmp = ((int32)(int16)motorRun * (int32)frqPuQ15 + (1 << 14)) >> 15;

#define    OUT_FDB_FRQ_DISP_FILTER_TIME   30 
    frqFdbLpf.t = OUT_FDB_FRQ_DISP_FILTER_TIME;
    frqFdbLpf.in = (frqFdbTmp);
    frqFdbLpf.calc(&frqFdbLpf);
    // 电机反馈速度
    frqFdbDispTmp = frqFdbLpf.out;

    // 第一次运行时编码器反馈速度方向检测
    checkSpeedFdbDir(frqFdbDispTmp);    

    if (funcCode.code.speedFdbDir != funcCode.code.runDir)
    {
        frqFdbDispTmp = -frqFdbDispTmp;   // 反馈速度取反作为正常速度
    }
    
    if(ABS_INT32(frqFdbDispTmp) > (Uint32)32000)
    {
        frqFdb = (frqFdbDispTmp > 0) ? (int16)32000 : (int16)-32000;
    }
    else
    {
        frqFdb = frqFdbDispTmp;
    }    

    // 功率因数角度
    gPhiRtDisp = (int32)gPhiRt*1800 >> 15;

#if DSP_2803X
    ABZPos = EQep1Regs.QPOSCNT & 0x0000FFFF;
#else
    if (motorFc.pgPara.elem.fvcPgSrc == FUNCCODE_fvcPgSrc_QEP2)
    {
        ABZPos = EQep2Regs.QPOSCNT & 0x0000FFFF;   
    }
    else
    {
        ABZPos = EQep1Regs.QPOSCNT & 0x0000FFFF;   
    }
#endif


    
#if DEBUG_F_INTERFACE
    if (speedMotor == frq2Core)  // 规避运算引起的误差
    {
        frqRun = frqDroop;
    }

    if(ABS_INT32(frqRun) > (Uint32)32000)
    {
        frqRunDisp = (frqRun > 0) ? (int16)32000 : (int16)-32000;
    }
    else
    {
        frqRunDisp = frqRun;
    }   
    
#if DEBUG_F_POSITION_CTRL
    pcOriginDisp = pcOrigin / 4;    // 显示4倍频之前
#endif

// 显示反馈速度，0.1rpm
//    funcCode.group.f2[18] = ((int32)(int16)frqFdb * ((int32)maxFrq + 2000) * 3 + (1 << 14)) >> 15;
// 显示反馈速度，0.01Hz
//    funcCode.group.f2[18] = ((int32)(int16)frqFdb * ((int32)maxFrq + 2000) + (1 << 14)) >> 15;
//    funcCode.group.f2[18] = frqRun;

    outCurrentDispOld = outCurrentDisp;    // 保存上一拍的输出电流，故障记录使用
//  currentInLpf.t = OUT_CURRENT_FILTER_TIME;   // 滤波时间系数
    tmp = ((int32)outCurrent * currentPu) >> 8;

#if 1
    if (ABS_INT32(outCurrentDispOldTmp - tmp) > 15) // 减小显示引起的波动. 15/16
    {
        outCurrentDisp = (tmp + (1 << 3)) >> 4;
        outCurrentDispOldTmp = tmp;
    }
#else    
    // 输出电流显示滤波
    currentInLpf.in = (tmp + (1 << 3)) >> 4;
    currentInLpf.calc(&currentInLpf);
    outCurrentDisp = currentInLpf.out;
#endif
    currentOcDisp = ((int32)currentOc * currentPu + (1 << 11)) >> 12;

    outVoltageDisp = ((int32)outVoltage * invPara.ratingVoltage + (1 << 11)) >> 12;

    // 软件过流
    softOcDoFlag = 0;
    if(runFlag.bit.run &&(funcCode.code.softOCValue) && (outCurrentDisp >= ((Uint32)funcCode.code.softOCValue*motorFc.motorPara.elem.ratingCurrent/1000)))
    {
        // 软件过流检测时间(过流点为零不检测)
        if(softOCTicker < ((Uint32)funcCode.code.softOCDelay*TIME_UNIT_CURRENT_CHK / CORE_FUNC_PERIOD))
        {
            softOCTicker++;
        }
        else
        {
            softOcDoFlag = 1;
            // errorOther = ERRROR_SOFT_OC;  // 软件过流
        }
    }
    else
    {
        softOCTicker = 0;
    }
    
#if DEBUG_F_POWER_ADD
    // 累计耗电量计算
    if (outPower > 0)   // 为负发电不回馈
    {
        powerAddDec += outPower;                 // 小数部分

        if (powerAddDec > POWER_ADD_COUNT_DEC)   // 整数部分+1
        {
            // 小数部分清0
            powerAddDec = 0;
            // 整数部分增1
            if (++funcCode.code.powerAddupInt >= POWER_ADD_COUNT_INT)
            {
                // 达到1度电
                funcCode.code.powerAddupInt = 0;
                // 累计耗电量增1
                funcCode.code.powerAddup++;
            }
        }
    }
#endif

    vcSpdLoopKp1 = funcCode.code.vcParaM1.vcSpdLoopKp1;
    vcSpdLoopKp2 = funcCode.code.vcParaM1.vcSpdLoopKp2;
#if DEBUG_F_POSITION_CTRL
    bPcErrorOk = 0;
    bPcErrorNear = 0;
#endif

    // bit0: 0-运行  1-停机
    // bit1、2:  0-恒速 1-加速  2-减速 
    // bit3: 0-正常 1-掉电
    invtStatus.bit.run = runFlag.bit.run;
    invtStatus.bit.accDecStatus = runFlag.bit.accDecStatus;
    invtStatus.bit.uv = bUv;
#endif

}


//=====================================================================
//
// 更新功能传递给性能的交互数据，0.5ms
//
//=====================================================================    
void UpdateDataFunc2Core0p5ms(void)
{
    int32 droopValue; // 下垂范围
	static Uint16 frq2CoreTmp;
    
// 下垂控制
    if (funcCode.code.droopCtrl 
        && frq 
        && (generatrixVoltage < DROOP_VOLTAGE_LEVEL) 
        && (runStatus != RUN_STATUS_TUNE))             // 不为调谐运行
    {
        droopValue = ((int64)torqueCurrentAct * funcCode.code.droopCtrl) / motorFc.motorPara.elem.ratingCurrent;
        frqDroop = frq - (int64)frq * droopValue / 1000;
    }
    else
    {
        frqDroop = frq;
    }
    
    frq2CoreTmp = (frqVFRunRemainder + (frqDroop << 15)) / ((int32)frqPuQ15);
#if 1
    if (((funcCode.code.ovGain) || (funcCode.code.ocGain))
            && (motorFc.motorCtrlMode == FUNCCODE_motorCtrlMode_VF)   // VF运行
            && (motorFc.motorPara.elem.motorType != MOTOR_TYPE_PMSM)  // 非同步机
            && (!funcCode.code.droopCtrl)                             // 下垂控制无效
            && (runStatus != RUN_STATUS_TUNE)                         // 不处于调谐状态
            && (runFlag.bit.accDecStatus == DEC_SPEED)
			&& (ABS_INT16((int16)frq2CoreTmp) >= ABS_INT16((int16)speedMotor))
        ) // 过压失速，过流失速增益都为0
    {
        //frq2Core = (frqVFRunRemainder + (frqDroop << 15)) / ((int32)frqPuQ15); 
    }
    else
    {
        frq2Core = frq2CoreTmp;
    }
#endif

#if DEBUG_F_INTERFACE  
#if !F_DEBUG_RAM
    copyDataFunc2Motor05ms();  // 功能传递给性能，0.5ms  
#endif
#endif
}


//=====================================================================
//
// 更新功能传递给性能的交互数据，2ms
//
//=====================================================================
extern Uint32 aptpSetOrigin;
void UpdateDataFunc2Core2ms(void)
{
#if DEBUG_F_INTERFACE      
    frqCurAim2Core = ((frqCurAim << 15) + frqCurAimFrac) / ((int32)frqPuQ15); // 当前的目标频率

    // 命令字
    // 给性能传递命令 0.5ms
    dspMainCmd.bit.motorCtrlMode = motorFc.motorCtrlMode;
    dspMainCmd.bit.accDecStatus = runFlag.bit.accDecStatus;     // 根据runFlag的加减速标志，给定dspMainCmd的加减速标志
    dspMainCmd.bit.torqueCtrl = funcCode.code.torqueCtrl;       // 转矩控制
    dspMainCmd.bit.spdLoopI1 = motorFc.vcPara.spdLoopI;         // 积分分离
    // 辅助命令字(2ms)    
    dspSubCmd.bit.outPhaseLossProtect = funcCode.code.outPhaseLossProtect;  // 输出缺相保护
    dspSubCmd.bit.inPhaseLossProtect = funcCode.code.inPhaseLossProtect%10; // 输入缺相保护
    dspSubCmd.bit.contactorMode = funcCode.code.inPhaseLossProtect/10;      // 接触器吸合保护
    dspSubCmd.bit.overloadMode = funcCode.code.overloadMode;                // 电机过载保护
    dspSubCmd.bit.loseLoadProtectMode = funcCode.code.loseLoadProtectMode;  // 输出掉载保护使能标志
    dspSubCmd.bit.poffTransitoryNoStop = funcCode.code.pOffTransitoryNoStop;// 瞬停不停
    //dspSubCmd.bit.overModulation = funcCode.code.overModulation;            // 过调制使能
    //dspSubCmd.bit.fieldWeak = funcCode.code.fieldWeak;                      // 弱磁控制
    dspSubCmd.bit.cbc = funcCode.code.cbcEnable;                            // 逐波限流使能
    //dspSubCmd.bit.narrowPulseMode = funcCode.code.narrowPulseMode;          // 窄脉冲控制选择
    //dspSubCmd.bit.currentSampleMode = funcCode.code.currentSampleMode;      // 电流检测滤波（剔除毛刺）选择
    dspSubCmd.bit.varFcByTem = funcCode.code.varFcByTem;                    // 载波频率随温度调整，MD280一直有效
    //dspSubCmd.bit.pmsmInitPosNoSame = funcCode.code.pmsmInitPosNoSame;
    //dspSubCmd.bit.pmsmZeroPosBig = funcCode.code.pmsmZeroPosBig;
    // 主命令字1(0.5ms)
    dspMainCmd1.bit.pgLocation = motorFc.pgPara.elem.fvcPgSrc;            // FVC的PG卡选择, 0-QEP1,1-QEP2(扩展)
    // 李润调整A0组功能码
    //dspMainCmd1.bit.pwmMode = funcCode.code.pwmMode;                // PWM模式选择. 
    dspMainCmd1.bit.modulationMode = funcCode.code.modulationMode;  // 调制方式
    dspMainCmd1.bit.deadCompMode = funcCode.code.deadCompMode;      // 死区补偿模式选择
    dspMainCmd1.bit.frqPoint = funcCode.code.frqPoint;              // 频率指令单位


    //jerkFf = (((int64)jerk * 32 * (int64)funcCode.code.servoKp2) << 15) / ((int32)maxFrq + 2000) / 100;

    driveCoeffFrqFdb = 1000;
#if !F_DEBUG_RAM

    // 过压抑制点计算
    //GetOverUdcPoint();
    copyDataFunc2Motor2ms();  // 功能传递给性能，2ms  
#else    
#endif

#if DEBUG_F_MOTOR_FUNCCODE
#if !F_DEBUG_RAM
// 调试使用功能码，func2motor 
    copyDataFunc2CF2ms();  // 功能传递给性能
#endif
#endif
#endif
}







