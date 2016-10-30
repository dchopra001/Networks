
#include <math.h>
#include <time.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>

using namespace std;

float tc;
int SN;
int RN;
int Next_Exp_Ack;
int Next_Exp_Frame;

int firstPacket = 1;

//specified parameters
float timeoutDelay;
int length;
int H;
int C;
float BER;
float tau;
int successfulFrames = 0;


struct Event 
   {
   float time;        //that event happens
   int errorFlag;      // 0, 1 or 2(no error, error, lost)
   int sequenceNumber; //0 or 1
   int type;           //0=timeout, 1=ACK
   };

//datraStructure
std::map<double, double> ES;
typedef map<double, double>::const_iterator MapIterator;
MapIterator it;
//it = buffer.begin buffer.end, key:it->first, value:it->second

float exponential_generator (float lambda)
   {
   float random = (float)rand()/((float)(RAND_MAX));
   float ret = (float)(-1 * (log(1 - random)/lambda));
   return ret;
   }

void printES()
   {
   it = ES.begin();
   while (it != ES.end())
      {
      double first = it->first;
      double second = it->second;
      printf("%f,%f\n",first,second);
      it++;
      }

   }
void purgeOldTimeout()
   {
   it = ES.begin();
   int cou = 0;
   int cou2 = 0;
   while(it != ES.end() && cou2 == 0)
      {
      printf("Round %i\n",cou);
      cou++;
      double currEvent = it->second;
      double key = it->first;
      if (currEvent == 6.7)
         {
         ES.erase(key);
         cou2 = 1;
         }
      else
         {
         it++;           //TODO:do you only increment if u didn't delete?
         printf("incremented it\n");
         printf("New Values: %f,%f\n",it->first,it->second);
         }
      }
      printf("Queue after checking:\n");
      printES();
   }



int main()
   {
   ES[1.1] = 4.3;
   ES[5.6] = 6.7;
   ES[4.7] = 9.8;
   ES[4.2] = 8.6;
   //printES();
   purgeOldTimeout();
   }
