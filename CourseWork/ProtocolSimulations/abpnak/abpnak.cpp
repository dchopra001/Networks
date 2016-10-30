#include <math.h>
#include <time.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
using namespace std;

double tc;
int SN;
int RN;
int Next_Exp_Ack;
int Next_Exp_Frame;

int firstPacket = 1;

//specified parameters
double timeoutDelay;
int length;
int H;
int C;
double BER;
double tau;
int successfulFrames = 0;


struct Event 
   {
   double time;        //that event happens
   int errorFlag;      // 0, 1 or 2(no error, error, lost)
   int sequenceNumber; //0 or 1
   int type;           //0=timeout, 1=ACK
   };

//datraStructure
std::map<double, Event> ES;
typedef map<double, Event>::const_iterator MapIterator;
MapIterator it;
//it = buffer.begin buffer.end, key:it->first, value:it->second

void resetVariables()
   {

   tc = 0.0;
   SN = 0;
   RN = 1;
   Next_Exp_Ack = 1;
   Next_Exp_Frame = 0;

   firstPacket = 1;

   successfulFrames = 0;
   }
float exponential_generator (float lambda)
   {
   float random = (float)rand()/((float)(RAND_MAX));
   float ret = (float)(-1 * (log(1 - random)/lambda));
   return ret;
   }

void purgeOldTimeout()
   {
   it = ES.begin();
   int done = 0;
   while(it != ES.end() && done == 0)
      {
      Event currEvent = it->second;
      double key = it->first;
      if (currEvent.type == 0)
         {
         ES.erase(key);
         done = 1;
         }
      else
         {
         it++;           //XX:do you only increment if u didn't delete?
         }
      }
   }
int channelIter = 0;
int channelSimulation(int numBits)
   {
   channelIter++;
   //printf("Entered ChannelSim\n");
   int numErrorBits = 0;
   for (int i = 0; i< numBits ; i++)
      {
      double random = ((double)rand())/((double)RAND_MAX);
      if (random < BER)
         {
         numErrorBits++;
         }
      }
   //printf("Exiting ChannelSim. channelIter: %i\n", channelIter);
   return numErrorBits; 
   }
int counter = 0;
void SEND()
   {
   //simulate forward channel
   int errorCount = channelSimulation(H + length);    
   //printf("ForwardChannel\n");
   //update tc here (prop. delay)
   tc = tc + tau;
   // now that transmission is done, simulate reciever
   //if lost:
   if (errorCount >= 5) 
      {
      //printf("Lost on FWD channel\n");
      return void();
      //do nothing
      }
   else if ((errorCount > 0 && errorCount <=4) || SN != Next_Exp_Frame) //if error in message or incorrect data recieved
      { 
      counter++;
      //printf("%i: Error on FWD channel\n", counter);
      if (SN != Next_Exp_Frame)
         {
        // printf("SN not equal nextExpFrame\n");
         }
 
      // prepare data for event creation later
      RN = Next_Exp_Frame;
      tc = tc +((double)H)/((double)C);
      
      }
   else if (errorCount == 0)
      {
      Next_Exp_Frame = (Next_Exp_Frame + 1)%2;
      RN = Next_Exp_Frame;
      tc = tc + ((double)H)/((double)C);
      }
   // simulate reverse channel 
   int errorCount2 = channelSimulation(H);
   tc = tc + tau;
   //printf("reverseChannel\n"); 
   //create acknowledgement event
   Event Ack = {tc,0,RN,1};
   //printf("RN:%i\n",RN);
   if (errorCount2 > 0  && errorCount2<=4)
      {
      Ack.errorFlag = 1;
      // insert event
      ES[Ack.time] = Ack;
      //printf("ERROR data on reverse channel\n");
      }
   else if (errorCount2 >=5)//we don't insert event if the packet is lost
      {
      Ack.errorFlag = 2;
      //printf("LOST data on reverse channel\n");
      }
   else //no error so insert in ES
      {
      ES[Ack.time] = Ack;
      //printf("NOERROR on reverse Channel\n");
      }
   }


void startSimulation()
   {
   it = ES.begin();
   while ((it != ES.end() || firstPacket == 1) && successfulFrames < 10000)
      {
      //printf("Back in loop\n");
      it = ES.begin();
      Event currEvent = it->second;
      if (firstPacket == 1)
         {
         //bookkeeping
         tc = 0;
         tc = tc + ((double)(((double)length) + ((double)H)))/((double)C);
         SN = 0;
         Next_Exp_Ack = (SN+1)%2;
         Next_Exp_Frame = 0;
         //update condition
         firstPacket = 0;
         //insert timeout event
         Event timeout = { tc+timeoutDelay, -1,-1,0};
         ES[timeout.time] = timeout;
         SEND();//simulate fwd,rcvr,rvrs
         } 
      else if (currEvent.type == 1 && currEvent.errorFlag == 0 && currEvent.sequenceNumber == Next_Exp_Ack)
         {
         //delete the top event:
         ES.erase(it->first);
         successfulFrames++;

         if (successfulFrames<10000)
            {
            SN = (SN+1)%2;
            Next_Exp_Ack = (SN+1)%2;
            tc =  tc + ((double)(((double)length) + ((double)H)))/((double)C);
            purgeOldTimeout();
            //insert new timeout event
            Event timeout = { tc+timeoutDelay, -1,-1,0};
            ES[timeout.time] = timeout;
            SEND(); //simulate fwd,rcvr,rvrs
            }
         }
      else if (currEvent.type == 1 && currEvent.errorFlag == 1) //error on rvrs channel
         {
         //printf("Error on RvrsChannel\n");
         //remove this event to trigger a timeout and resend packet for acknowledgement
         ES.erase(it->first);
         tc = tc + ((double)(((double)length) + ((double)H)))/((double)C);
         purgeOldTimeout();
         Event timeout = { tc+timeoutDelay, -1,-1,0};
         ES[timeout.time] = timeout;
         SEND(); //simulate fwd,rcvr,rvrs
         }
      else if (currEvent.type == 1 && currEvent.sequenceNumber != Next_Exp_Ack)
         {
         //do nothing for now...
         //delete the top event:
         ES.erase(it->first);
         tc = tc + ((double)(((double)length) + ((double)H)))/((double)C);
         purgeOldTimeout();
         Event timeout = { tc+timeoutDelay, -1,-1,0};
         ES[timeout.time] = timeout;
         SEND(); //simulate fwd,rcvr,rvrs
         }
      else if (currEvent.type == 0)// ||(currEvent.type == 1 && (currEvent.errorFlag == 1 ||currEvent.sequenceNumber != Next_Exp_Ack))) //TODO: Add an or here for question 2?
         {
        // printf("Found a timeout now\n");
         //delete the top event:
         tc = it->first;
         ES.erase(it->first);
         tc = tc + ((double)(((double)length) + ((double)H)))/((double)C);
         //purgeOldTimeout();
         ES.clear();
         //insert new timeout event
         Event timeout = { tc+timeoutDelay, -1,-1,0};
         ES[timeout.time] = timeout;
         SEND(); //simulate fwd,rcvr,rvrs
         
         }

      it = ES.begin();  
      } 
   }

int main(int argc, char **argv)
   {

   //argument format: H(bytes)  length(bytes)  timeoutDelay  C  tau  BER
   //if (argc<=6)
     // {
     // printf("Invalid Number of Arguments\n");
     // return 1;
     // } 
   srand(time(NULL));
  /* H = atoi(argv[1]) * 8;
   length = atoi(argv[2]) * 8;
   timeoutDelay = atof(argv[3]);
   C = atoi(argv[4]);
   tau = atof(argv[5]);
   BER = atof(argv[6]);
*/
   double a1,a2,a3,a4,a5,a6;
   double b1,b2,b3,b4,b5,b6;
   double c1,c2,c3,c4,c5,c6;
   double d1,d2,d3,d4,d5,d6;
   double e1,e2,e3,e4,e5,e6;

   H = 54 * 8;
   length=1500*8;
   C = 5000000;

   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 2.5;
   BER = 0.0;
   startSimulation();
   a1 = (double)(successfulFrames*length)/tc;
   resetVariables();
   //timeoutDelay, tau, BER
   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 2.5;
   BER = 0.00001;
   startSimulation();
   a2 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 2.5;
   BER = 0.0001;
   startSimulation();
   a3 = (double)(successfulFrames*length)/tc;


   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 2.5;
   BER = 0.0;
   startSimulation();
   a4 = (double)(successfulFrames*length)/tc;
   
   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 2.5;
   BER = 0.00001;
   startSimulation();
   a5 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 2.5;
   BER = 0.0001;
   startSimulation();
   a6 = (double)(successfulFrames*length)/tc;





   resetVariables();

   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 5;
   BER = 0.0;
   startSimulation();
   b1 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //timeoutDelay, tau, BER
   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 5;
   BER = 0.00001;
   startSimulation();
   b2 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 5;
   BER = 0.0001;
   startSimulation();
   b3 = (double)(successfulFrames*length)/tc;


   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 5;
   BER = 0.0;
   startSimulation();
   b4 = (double)(successfulFrames*length)/tc;
   
   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 5;
   BER = 0.00001;
   startSimulation();
   b5 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 5;
   BER = 0.0001;
   startSimulation();
   b6 = (double)(successfulFrames*length)/tc;


   resetVariables();




   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 7.5;
   BER = 0.0;
   startSimulation();
   c1 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //timeoutDelay, tau, BER
   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 7.5;
   BER = 0.00001;
   startSimulation();
   c2 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 7.5;
   BER = 0.0001;
   startSimulation();
   c3 = (double)(successfulFrames*length)/tc;


   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 7.5;
   BER = 0.0;
   startSimulation();
   c4 = (double)(successfulFrames*length)/tc;
   
   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 7.5;
   BER = 0.00001;
   startSimulation();
   c5 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 7.5;
   BER = 0.0001;
   startSimulation();
   c6 = (double)(successfulFrames*length)/tc;


   //timeoutDelay = timeoutDelay * tau;

   resetVariables();








   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 10;
   BER = 0.0;
   startSimulation();
   d1 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //timeoutDelay, tau, BER
   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 10;
   BER = 0.00001;
   startSimulation();
   d2 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 10;
   BER = 0.0001;
   startSimulation();
   d3 = (double)(successfulFrames*length)/tc;


   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 10;
   BER = 0.0;
   startSimulation();
   d4 = (double)(successfulFrames*length)/tc;
   
   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 10;
   BER = 0.00001;
   startSimulation();
   d5 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 10;
   BER = 0.0001;
   startSimulation();
   d6 = (double)(successfulFrames*length)/tc;


   resetVariables();





   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 12.5;
   BER = 0.0;
   startSimulation();
   e1 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //timeoutDelay, tau, BER
   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 12.5;
   BER = 0.00001;
   startSimulation();
   e2 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //row1, column1
   tau = 0.005;
   timeoutDelay = tau * 12.5;
   BER = 0.0001;
   startSimulation();
   e3 = (double)(successfulFrames*length)/tc;

   resetVariables();

   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 12.5;
   BER = 0.0;
   startSimulation();
   e4 = (double)(successfulFrames*length)/tc;
   
   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 12.5;
   BER = 0.00001;
   startSimulation();
   e5 = (double)(successfulFrames*length)/tc;

   resetVariables();
   //row1, column1
   tau = 0.25;
   timeoutDelay = tau * 12.5;
   BER = 0.0001;
   startSimulation();
   e6 = (double)(successfulFrames*length)/tc;

   resetVariables();
   ofstream result_file;
   result_file.open("ABP_NAK.csv");
   result_file << a1<<","<<a2<<","<<a3<<","<<a4<<","<<a5<<","<<a6<<"\n";
   result_file << b1<<","<<b2<<","<<b3<<","<<b4<<","<<b5<<","<<b6<<"\n";
   result_file << c1<<","<<c2<<","<<c3<<","<<c4<<","<<c5<<","<<c6<<"\n";
   result_file << d1<<","<<d2<<","<<d3<<","<<d4<<","<<d5<<","<<d6<<"\n";
   result_file << e1<<","<<e2<<","<<e3<<","<<e4<<","<<e5<<","<<e6<<"\n";
   //printf("H: %i, length: %i, timeoutDelay: %f, C: %i, tau: %f, BER: %f\n",H,length,timeoutDelay,C,tau,BER);

   //startSimulation();
   //printf("Total Simulation Time: %f\n",tc);
   //printf("SuccessFrames: %i\n",successfulFrames);
   //printf("Total Bits delivered: %i\n",successfulFrames*(length));
   //printf("Throughput: %f\n",(double)(successfulFrames*length)/tc);

/*   printf("Sending value: 2\n");
   int answer = multiplyByTwo(2);
   printf("Result: %i\n",answer);

   printf("Testing Pointers\n");
   int *ip;
   int var = 5;

   ip = &var;
   printf("ip = %i, &var = %i\n",ip, &var); //address
   printf("*ip= %i, var = %i\n",*ip, var);  //value
   printf("&ip = %i\n",&ip);                //address
  
   printf("Testing Stucture\n");
   Test1 Test;
   Test.x_value = 5;
   Test.y_value = 10;
   Test1 *ip;
   ip = &Test;

   printf("ip=%i, &Test = %i\n",ip,&Test); //address
   printf("*ip.x_value = %i, *ip.y_value = %i, Test.x_value = %i, Test.y_value = %i\n",ip->x_value, ip->y_value, Test.x_value, Test.y_value);
*/
   return 0;
   }
