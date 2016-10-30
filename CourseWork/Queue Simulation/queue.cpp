#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <time.h>
#include <string>
#include <map>
#include <limits>

using namespace std;

float exponential_generator (float lambda)
   {
      float random = (float)rand()/((float)(RAND_MAX));
      float ret = (float)(-1 * (log(1 - random)/lambda));
      return ret;
   }

void generateMeanAndVariance()
   {
   float lambda = 75;
   float array[1000];
   float value;
   float sum;
   //generate 1000 random variables and store in array
   for (int i = 0; i < 1000; i++) 
      {
      value = exponential_generator(lambda);
      sum += value;
      array[i] = value;
      }
    //find mean
   float mean = sum / 1000;
   //sum for variance
   sum = 0;
   for (int i = 0; i < 1000; i++) 
      {
      sum += pow(array[i]-mean, 2);
      }
   //find variance
   float variance = sum / (1000-1);
   cout<<"Variance: "<<variance<<" mean: "<<mean<<endl;
   }
//bufferSize, Ngen, Nloss
float * getData(float result[], float simTime, int bufferSize, float transRate, float avgLength, float ro )
   {
   int infiniteBuffer = 0;
   srand(time(NULL));


   float Na = 0; //number of arrivals in the queue
   float Nd = 0; //Number of departures
   float No = 0; //Number of observers
   float Ni = 0; //Idle counter
   float Np = 0; //Number of packets in the system counter


   float Ngen = 0; //Number of packets during simulation
   float Nloss = 0; //Number of lost packets


   float ta = 0; //Arrival Time
   float td = 0;  //departure time
   float to = 0; //observation time

   float k = std::numeric_limits<float>::infinity(); //buffer size, int is inherently infinite
   if (bufferSize != -1)
      {
      k = bufferSize;
      }
   float simulation_time = simTime;//10000;
   float C = transRate;// 1000000; //transmission rate of the output link in bits per second
   float L = avgLength;//12000; //average length of packets in bits
   float rho = ro;//0.95;
   
   float lambda = (rho*C)/((float)L);
   float alpha = 3 * lambda;

   float packet_length;
   float service_time;
   

   std::map<float,string> buffer;
   typedef map<float, string>::const_iterator MapIterator;
   MapIterator it;

   //Generation of arrivals
   while (ta < simulation_time)
      {
      ta = ta + exponential_generator(lambda);
                                                                     ////TODO:Insert arrival event at time ta in DES --DONE-------------------------
      buffer[ta] = "arrival";
      }
   //Generation of observations
   while (to < simulation_time)
      {
      to = to + exponential_generator(alpha);
                                                                       ////TODO:Insert observation event at time to in DES --DONE------------------------------
      buffer[to] = "observation";
      }
  
   // We will generate departure packets dynamically
   it = buffer.begin();
   while (it != buffer.end())                                                                       ////TODO: Add condition : "DES IS NOT EMPTY" --DONE----------------------
      {
      string event_type;
      float event_time; 
                                                                      ////TODO: Read the first event in DES as event_type and event_time --DONE--------------------
      event_time = it->first;
      event_type = it->second; 
      if (event_type == "arrival")
         {
         Ngen = Ngen + 1; //increment the generated packets by one
         
         if (Na - Nd < k) //queue is not full                         || infiniteBuffer == 1) // queue is not full
            {
            packet_length = exponential_generator(((float)1)/L);
            service_time = packet_length/C;

            if (Na - Nd == 0) //No or Na? ... queue is empty
               {
               td = event_time + service_time;
               }
            else //queue is not empty
               {
               td = td + service_time;
               }
                                                                                   ////TODO:Insert departure event at time td in DES --DONE------------------
            buffer[td] = "departure";
            Na = Na + 1;
                                                                              ////TODO: Delete the first event in DES (arrival)  --DONE------------------
            buffer.erase(event_time);
            }
     
         else //queue is full
            {
            Nloss = Nloss + 1; //increment number of lost packets
                                                                                              ////TODO: Delete the first event in DES  --DONE-------------
            buffer.erase(event_time);
            }
         }

      else if (event_type == "departure")
         {
         Nd = Nd + 1; //increment departure counter
                                                                            ////TODO: Delete the first event in DES (departure) --DONE-------------
         buffer.erase(event_time); 
         }

      else if (event_type == "observation")
         {
         No = No + 1; //increment the observation counter
         Np = Np + (Na-Nd); //count how many packets are in the queue
        
         if (Na - Nd == 0) // queue is idle
            {
            Ni = Ni + 1; //increment the idle counter
            }
                                                                                    ////TODO: Delete the first event in DES (observation) --DONE----------------
         buffer.erase(event_time); 
         }
      it = buffer.begin();
      }
   float avgNumPackets = ((float)Np)/((float)No);
   float idleProbability = ((float)Ni)/((float)No);
   float probLoss = ((float)Nloss)/((float)Ngen);
   result[0] = avgNumPackets;
   result[1] = idleProbability;
   result[2] = probLoss;
   return result;
   }

void executeQuestion(float simTime, int bufferSize, float transRate, float avgLength, float roLower, float roUpper, float roIncrement, int iterationsPerRo)
   {
   float En = 0;
   float Pidle = 0;
   float Ploss = 0;

   float result[3];
   float *result2;
   //getData(float result[], float simTime, int bufferSize, float transRate, float avgLength, float ro );
   int rowCount = 1;
   for (float i = roLower; i<=roUpper; i = i + roIncrement)
      {
      En = 0;
      Pidle = 0;
      Ploss = 0;
      for (int j = 0; j<iterationsPerRo; j++)
         {
         result2 = getData(result,simTime, bufferSize,transRate,avgLength,i);
         En = En + result2[0];
         Pidle = Pidle + result2[1];
         Ploss = Ploss + result2[2]; 
         }

      cout<<rowCount<<". rho = "<<i<<", En="<<En/iterationsPerRo<<",Pidle="<<Pidle/iterationsPerRo<<",Ploss="<<Ploss/iterationsPerRo<<endl;
      rowCount++;
      }
   }

 
int main()
   {
   //-------------------------------Question 3.1 and 3.2--------------------------------//
   //
   generateMeanAndVariance(); //finite queue simulation
   executeQuestion((float)10000,5,(float)1000000,(float)12000,(float)0.25,(float)0.34,(float)0.1,1);
   return 1;
  /* cout<<"bufferSize=5"<<endl;
   executeQuestion((float)10000,5,(float)1000000,(float)12000,(float)0.4,(float)2.01,(float)0.1,1);
   executeQuestion((float)10000,5,(float)1000000,(float)12000,(float)2.2,(float)5.01,(float)0.2,1);
   executeQuestion((float)10000,5,(float)1000000,(float)12000,(float)5.4,(float)10.01,(float)0.4,1);
   cout<<"bufferSize=10"<<endl;
   executeQuestion((float)10000,10,(float)1000000,(float)12000,(float)0.4,(float)2.01,(float)0.1,1);
   executeQuestion((float)10000,10,(float)1000000,(float)12000,(float)2.2,(float)5.01,(float)0.2,1);
   executeQuestion((float)10000,10,(float)1000000,(float)12000,(float)5.4,(float)10.01,(float)0.4,1);
   cout<<"bufferSize=40"<<endl;
   executeQuestion((float)10000,40,(float)1000000,(float)12000,(float)0.4,(float)2.01,(float)0.1,1);
   executeQuestion((float)10000,40,(float)1000000,(float)12000,(float)2.2,(float)5.01,(float)0.2,1);
   executeQuestion((float)10000,40,(float)1000000,(float)12000,(float)5.4,(float)10.01,(float)0.4,1);

 
   return 1;*/
   }
