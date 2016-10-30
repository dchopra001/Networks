#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <time.h>
#include <string>
#include <map>
using namespace std;

int main()
   {
   std::map<float,string> test;
   test[0.95] = "arrival";
   test[0.73] ="departure";
   test[0.87] = "observer";
 
   cout << "Size: "<<test.size()<<endl;
   typedef map<float, string>::const_iterator MapIterator;
   MapIterator it = test.begin();
   cout<<"First KEY,Value: "<<it->first<<","<<it->second<<endl;
   it++;
   cout<<"Second KEY,Value: "<<it->first<<","<<it->second<<endl;
   it++;
   cout<<"Last KEY,Value: "<<it->first<<","<<it->second<<endl;
   it = test.end();
   it--;
   cout<<"Last KEY,Value: "<<it->first<<","<<it->second<<endl;
   it = test.begin();
   cout<<"LOOP"<<endl;
   while(it != test.end())
      {
      cout<<"Last KEY,Value: "<<it->first<<","<<it->second<<endl;
      it++;

      }

   it=test.begin();
   test.erase(it->first);
   it = test.begin();

   cout<<"LOOP2"<<endl;
   while(it != test.end())
      {
      cout<<"Last KEY,Value: "<<it->first<<","<<it->second<<endl;
      it++;

      }

   test[0.96] = "departure";
   it=test.begin();
   test.erase(it->first);
   it = test.begin();

   cout<<"LOOP3"<<endl;
   while(it != test.end())
      {
      cout<<"Last KEY,Value: "<<it->first<<","<<it->second<<endl;
      it++;

      }


   it=test.begin();
   test.erase(it->first);
   it = test.begin();

   cout<<"LOOP4"<<endl;
   while(it != test.end())
      {
      cout<<"Last KEY,Value: "<<it->first<<","<<it->second<<endl;
      it++;

      }

   it=test.begin();
   test.erase(it->first);
   it = test.begin();

   cout<<"LOOP5"<<endl;
   while(it != test.end())
      {
      cout<<"Last KEY,Value: "<<it->first<<","<<it->second<<endl;
      it++;

      }

   it=test.begin();
   test.erase(it->first);
   it = test.begin();

   cout<<"LOOP6"<<endl;
   while(it != test.end())
      {
      cout<<"Last KEY,Value: "<<it->first<<","<<it->second<<endl;
      it++;

      }


   if (5+6==11 && 6+7==13)
      {
      cout<<"Works"<<endl;
      }
   }

