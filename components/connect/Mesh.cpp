/*
 * Copyright (C) 2024 Commissariat à l'énergie atomique et aux énergies alternatives (CEA)

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *    http://www.apache.org/licenses/LICENSE-2.0 

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "Mesh.hpp"

using namespace vpsim;

//builds a routing that sends request in the X axis then the Y axis
void C_Mesh::BuildRoutingXY()
{
	for(unsigned int i=0;i<SizeX;i++)
		for(unsigned int j=0;j<SizeY;j++)
		{
			//build router
			unsigned int RouterId=i*SizeY+j;
			//build XY routing map:
			for(unsigned int ii=0;ii<SizeX;ii++)
				for(unsigned int jj=0;jj<SizeY;jj++)
				{
					//no self routing
					if(ii==i && jj==j)
						continue;

					unsigned int TargetId= ii*SizeY+jj;
					unsigned int OutPort;

					//X routing first
					if(ii<i){
						OutPort=3;
					}
					else if(ii>i)
					{
						OutPort=1;
					}
					else{

						//Y routing second
						if(jj<j){
							OutPort=2;
						}
						else if(jj>j)
						{
							OutPort=0;
						}
						else{
							SYSTEMC_ERROR("undefined case, should not occur")
						}
					}

					AddRouting(RouterId,TargetId,OutPort);

				}//end for all targets
		}//end for all sources
}

C_Mesh::C_Mesh(sc_module_name name, unsigned int _SizeX, unsigned int _SizeY):C_NoCBase(name),C_NoC(name)
{

	//		 __ __ __ __
	//	  ^	|  |  |  |  |
	//	s |	|__|__|__|__|
	//	i |	|  |  |  |  |
	//	z |	|__|__|__|__|
	//	e |	|  |  |  |  |
	//	Y |	|__|__|__|__|
	//          sizeX
	//		---------->


	SizeX=_SizeX;
	SizeY=_SizeY;
	RoutingDone=false;


	if(SizeX==1 && SizeY==1)
		AddRouter(0); //necessary only for unconnected router

	//unsigned int MaxRouterId=SizeX*SizeY;
	for(unsigned int i=0;i<SizeX;i++)
		for(unsigned int j=0;j<SizeY;j++)
		{
			//build router
			unsigned int RouterId=i*SizeY+j;
			//AddRouter(RouterId); //necessary only for unconnected router

			//build links
			//out ports are numbered clockwise starting from north
			//							North(port0)
			//								^
			//								|
			//			West (port3)<--- Router ----> East (port1)
			//								|
			//								v
			//							North(port0)

			unsigned int NorthID= RouterId+1; //port 0
			unsigned int SouthID= RouterId-1; //port 2
			unsigned int EastID= RouterId+SizeY; //port 1
			unsigned int WestID= RouterId-SizeY; //port 3

			//North/East/South/West ID can go over the number of routers
			if(j+1<SizeY)
				AddLink(RouterId,NorthID,debug);
			if(j>=1)
				AddLink(RouterId,SouthID,debug);
			if(i+1<SizeX)
				AddLink(RouterId,EastID,debug);
			if(i>=1)
				AddLink(RouterId,WestID,debug);
		}
}//end constructor


void C_Mesh::BuildRouting(E_RoutingMode RMode)
{
	//build routing map
	if(RMode==XY){
		BuildRoutingXY();
	}
	else{
		C_NoC::BuildDefaultRoutingBidirectional();
	}

	RoutingDone=true;

}
