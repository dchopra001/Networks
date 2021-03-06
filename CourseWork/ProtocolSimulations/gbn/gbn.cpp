#include <math.h>
#include <time.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace std;



int startOfBuffer;
int endOfBuffer;
int currentFrameToSend;
int numberOfFramesToSend;

int sampleSize[10000];
double T[10000];

int windowSize;

double tc;
int SN;
int RN;
int Next_Exp_Ack;
int Next_Exp_Frame;

int firstPacket = 1;

//specified parameters
double timeoutDelay;
int length;
int finalLength;
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
   for (int i=0; i<10000; i++)
      {
      T[i] = 0;
      }
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
   //printf("%i\n", channelIter);
   return numErrorBits; 
   }
int counter = 0;
void SEND()
   {
   double tempTC = tc;
   //printf("1. %f\n",tc);
   //simulate forward channel
   int errorCount = channelSimulation(H + length);    
   //printf("ForwardChannel\n");
   //update tc here (prop. delay)
   tempTC = tempTC + tau;
   // now that transmission is done, simulate reciever
   //printf("2. %f\n",tc);
   //if lost:
   if (errorCount >= 5) 
      {
      //printf("Lost on FWD channel\n");
      return void();
      //do nothing
      }
   else if ((errorCount > 0 && errorCount <=4) || currentFrameToSend != Next_Exp_Frame) //if error in message or incorrect data recieved
      { 
      counter++;
      //printf("%i: Error on FWD channel\n", counter);
      if (currentFrameToSend != Next_Exp_Frame)
         {
        // printf("SN not equal nextExpFrame\n");
         }
 
      // prepare data for event creation later
      RN = Next_Exp_Frame;
      tempTC = tempTC +((double)H)/((double)C);
      
 //  printf("3. %f\n",tc);
      }
   else if (errorCount == 0)
      {
      Next_Exp_Frame = (Next_Exp_Frame + 1);
      RN = Next_Exp_Frame;
      tempTC = tempTC + ((double)H)/((double)C);
  // printf("4. %f\n",tc);
      }
   // simulate reverse channel 
   int errorCount2 = channelSimulation(H);
   tempTC = tempTC + tau;
   //printf("5. %f\n",tc);
   //printf("reverseChannel\n"); 
   //create acknowledgement event
   Event Ack = {tempTC,0,RN,1};
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
     // printf("InsertingAck at %f, RN: %i\n", tempTC, RN);
     // printf("Inserted Event\n");
      ES[Ack.time] = Ack;
      //printf("NOERROR on reverse Channel\n");
      }
   }

void Sender()
   {
   int senderCount=0;
   while (currentFrameToSend<=endOfBuffer) //while buffer has frames to send
      {
      tc = tc + ((double)finalLength)/((double)C);
      //printf("Iteration %i of Sender at time:  %f\n",senderCount, tc);
      senderCount++;
      T[currentFrameToSend] = tc;
      Next_Exp_Ack = currentFrameToSend + 1;

      if (currentFrameToSend == startOfBuffer) //oldest frame in the buffer
         {
        // printf("OldestFrame is current frame to send\n");
         purgeOldTimeout();
         Event timeout = {T[currentFrameToSend] + timeoutDelay,-1,-1,0}; 
         ES[timeout.time] = timeout;
        // printf("Inserting timeout because new beginning of buffer: %f\n", timeout.time);
         } 
      SEND(); //send the packet
      it = ES.begin();
      Event currEvent = it->second;
      if (currEvent.time < T[currentFrameToSend] && currEvent.type == 0) //timeout event happened during transmission
         {
         //printf("Found a timeout in the Sender\n");
         currentFrameToSend = startOfBuffer; //start retransmitting from start of buffer again
         purgeOldTimeout();
         ES.clear();
         continue;
         }
      // error free ack event happened during transmission
      if (currEvent.time < T[currentFrameToSend] && currEvent.type == 1 && (currEvent.sequenceNumber <= currentFrameToSend))
         {
        // printf("Removing Ack: %i\n", currEvent.sequenceNumber);
         double eventTime = it->first;
         ES.erase(it->first);
         //printf("Found an ack in the sender\n");
         // need to slide here...
         successfulFrames = successfulFrames + (currEvent.sequenceNumber - startOfBuffer);
         startOfBuffer = currEvent.sequenceNumber;
         endOfBuffer = startOfBuffer + windowSize - 1; //3
         if (endOfBuffer >= numberOfFramesToSend)
            {
           // printf("In first condition\n");
            endOfBuffer = numberOfFramesToSend - 1;
            }
         if (currentFrameToSend < startOfBuffer)
            {
            //printf("In Second condition\n");
            currentFrameToSend = startOfBuffer;
            }
         else //if currentFrameToSend is NOT reset, then just update timeout.. otherwise timeout will be updated in the next round anyway
            {
          //  printf("Inserting Timeout in Sender\n");
            //printf("In third condition\n");
            //printf("TimeOfFirstEventInBuffer:%f\n",T[startOfBuffer]);
            purgeOldTimeout();
            Event timeout = {T[startOfBuffer] + timeoutDelay,-1,-1,0}; 
            ES[timeout.time] = timeout;
            //printf("Exiting...\n");
            }
         if (successfulFrames == numberOfFramesToSend)
            {
            tc = eventTime; //final ack time is the total sim time  
            break;
            }
         }
      currentFrameToSend = currentFrameToSend + 1;

      }
   }



void startSimulation()
   {
   it = ES.begin();
   while ((it != ES.end() || firstPacket == 1) && successfulFrames <= numberOfFramesToSend)
      {
      //printf("In ES Processor at tc:%f\n",tc);
      tc = it->first;
      Event currEvent = it->second;
      if (firstPacket == 1)
         {
         firstPacket = 0;
         //printf("First Packet...\n");
         //initialize
         startOfBuffer = 0;
         endOfBuffer = startOfBuffer + windowSize - 1;  //i3
         if (numberOfFramesToSend < windowSize)
            {
            endOfBuffer = numberOfFramesToSend;
            }
         currentFrameToSend = 0;
         Next_Exp_Ack = 1;
         Next_Exp_Frame = 0;
         tc = 0;
         Sender(); //send all frames
         }
      else if (currEvent.type == 0) //timeout
         {
         //printf("Found a Timeout in the ES Processor\n");
         currentFrameToSend = startOfBuffer; //start retransmitting from start of buffer again
         purgeOldTimeout();
         ES.clear();
         Sender(); //resend all frames
         }
      else if (currEvent.type == 1 && currEvent.sequenceNumber != startOfBuffer -1)
         {
         //printf("Removing ACK found in ES: %i\n", currEvent.sequenceNumber);
         double finalEventTime = it->first;
         ES.erase(it->first);
         //printf("Found an ACK in the ES Processor\n");
         // need to slide here...
         successfulFrames = successfulFrames + (currEvent.sequenceNumber - startOfBuffer);
         startOfBuffer = currEvent.sequenceNumber;
         endOfBuffer = startOfBuffer + windowSize - 1; //3
         if (endOfBuffer >= numberOfFramesToSend)
            {
            endOfBuffer = numberOfFramesToSend - 1;
            }
         if (currentFrameToSend < startOfBuffer)  //if all frames in window have been acknowledged
            {
            currentFrameToSend = startOfBuffer;
            }
         else //otherwise all frames  weren't set so update the timeout
            {
           // printf("timeoutDelay:%f\n",timeoutDelay);
           // printf("Inserting Timeout in ES at : %f\n", T[startOfBuffer] + timeoutDelay);
            purgeOldTimeout();
            
            Event timeout = {T[startOfBuffer] + timeoutDelay,-1,-1,0}; 
            ES[timeout.time] = timeout;
            }
         if (numberOfFramesToSend == successfulFrames)
            {
            tc = finalEventTime;
           // printf("FinalEventTime: %f\n", tc);
            break;
            }
         Sender();
         }
      
      it = ES.begin();  
      } 
   }

int main(int argc, char **argv)
   {
   srand(time(NULL));
   windowSize = 4;
   

 double a1,a2,a3,a4,a5,a6;
   double b1,b2,b3,b4,b5,b6;
   double c1,c2,c3,c4,c5,c6;
   double d1,d2,d3,d4,d5,d6;
   double e1,e2,e3,e4,e5,e6;

   H = 54 * 8;
   length=1500*8;
   C = 5000000;
   finalLength = H + length;
   numberOfFramesToSend = 10000;

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
   result_file.open("GBN.csv");
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



   //printf("H: %i, length: %i, timeoutDelay: %f, C: %i, tau: %f, BER: %f\n",H,length,timeoutDelay,C,tau,BER);

   return 0;
   }
