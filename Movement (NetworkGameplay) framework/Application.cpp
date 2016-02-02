#include "Application.h"
#include "ship.h"
#include "Globals.h"
#include "MyMsgIDs.h"
#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "Bitstream.h"
#include "GetTime.h"
#include"MyMath.h"
#include <hge.h>
#include <string>
#include <iostream>
#include <hgeSprite.h>
#include <fstream>



// Lab 13 Task 9a : Uncomment the macro NETWORKMISSILE
#define NETWORKMISSILE


float GetAbsoluteMag( float num )
{
	if ( num < 0 )
	{
		return -num;
	}

	return num;
}

/** 
* Constuctor
*
* Creates an instance of the graphics engine and network engine
*/

Application::Application() 
:	hge_(hgeCreate(HGE_VERSION))
,	rakpeer_(RakNetworkFactory::GetRakPeerInterface())
,	timer_( 0 )
// Lab 13 Task 2 : add new initializations
{
}

/**
* Destructor
*
* Does nothing in particular apart from calling Shutdown
*/

Application::~Application() throw()
{
	Shutdown();
	rakpeer_->Shutdown(100);
	RakNetworkFactory::DestroyRakPeerInterface(rakpeer_);
}

/**
* Initialises the graphics system
* It should also initialise the network system
*/

bool Application::Init()
{
	std::ifstream inData;	
	std::string serverip;

	inData.open("serverip.txt");

	inData >> serverip;

	srand( RakNet::GetTime() );

	hge_->System_SetState(HGE_FRAMEFUNC, Application::Loop);
	hge_->System_SetState(HGE_WINDOWED, true);
	hge_->System_SetState(HGE_USESOUND, false);
	hge_->System_SetState(HGE_TITLE, "Movement");
	hge_->System_SetState(HGE_LOGFILE, "movement.log");
	hge_->System_SetState(HGE_DONTSUSPEND, true);



	if(hge_->System_Initiate()) 
	{
		shipsList.push_back(new Ship(rand()%4+1, rand()%500, rand()%400+100));
		shipsList.at(0)->SetName("My Ship");

		HTEXTURE test = hge_->Texture_Load("Background.png");
		backgroundSprite.reset(new hgeSprite(test, 0, 0, 800, 600));

		if (rakpeer_->Startup(1,30,&SocketDescriptor(), 1))
		{
			rakpeer_->SetOccasionalPing(true);
			return rakpeer_->Connect(serverip.c_str(), 1691, 0, 0);
		}
	}

	return false;
}

/**
* Update cycle
*
* Checks for keypresses:
*   - Esc - Quits the game
*   - Left - Rotates ship left
*   - Right - Rotates ship right
*   - Up - Accelerates the ship
*   - Down - Deccelerates the ship
*
* Also calls Update() on all the ships in the universe
*/
bool Application::Update()
{
	if (hge_->Input_GetKeyState(HGEK_ESCAPE))
		return true;

	float timedelta = hge_->Timer_GetDelta();

	playerControl(timedelta);

	/*
		if (asteroidList.size() < 20)
			{
				float x = Math::RandFloatMinMax(0, 800);
				float y = Math::RandFloatMinMax(0, 600);
				float w = Math::RandFloatMinMax(0, 360);
				CreateAsteroid(x, y, w);
			}*/

	for (std::vector<Ship*>::iterator it = shipsList.begin(); it != shipsList.end(); ++it)
	{
		Ship* ship = static_cast<Ship*>(*it);
		ship->Update(timedelta);

		if (ship == shipsList.at(0))
		{
			checkCollisions(ship);
		}
	}

	for (std::vector<Laser*>::iterator it = missileList.begin(); it != missileList.end(); ++it)
	{
		Laser* missile = static_cast<Laser*>(*it);

		if (missile->Update(shipsList, timedelta))
		{
			delete *it;
			missileList.erase(it);
			break;
		}
	}

	for (std::vector<CAsteroid*>::iterator it = asteroidList.begin(); it != asteroidList.end(); ++it)
	{
		CAsteroid* asteroid = static_cast<CAsteroid*>(*it);
		asteroid->Update(timedelta);
	}

	//for (std::vector<CAsteroid*>::iterator it = asteroidList.begin(); it != asteroidList.end(); ++it)
	//{
	//	CAsteroid* asteroid = static_cast<CAsteroid*>(*it);
	//	asteroid->Update(timedelta);

	//	//asteroid->Update(timedelta);

	//	/*if (asteroid->UpdateShip(shipsList, timedelta))
	//	{
	//		delete *it;
	//		asteroidList.erase(it);
	//		break;
	//	}
	//	else if (asteroid->UpdateLaser(missileList))
	//	{
	//		if (asteroid->isSmall())
	//		{
	//			delete *it;
	//			asteroidList.erase(it);
	//		}
	//		else
	//		{
	//			for (int a = 0; a < 2; ++a)
	//			{
	//				float x = Math::RandFloatMinMax(asteroid->GetX() - 20, asteroid->GetX() + 20);
	//				float y = Math::RandFloatMinMax(asteroid->GetY() - 20, asteroid->GetY() + 20);
	//				float w = Math::RandFloatMinMax(asteroid->GetW() - 20, asteroid->GetW() + 20);
	//				CreateAsteroid(x, y, w, true);
	//			}
	//			delete *it;
	//			asteroidList.erase(it);
	//			break;
	//		}
	//		break;
	//	}*/
	//}

	if (Packet* packet = rakpeer_->Receive())
	{
		RakNet::BitStream bs(packet->data, packet->length, false);
		
		unsigned char msgid = 0;
		RakNetTime timestamp = 0;

		bs.Read(msgid);

		if (msgid == ID_TIMESTAMP)
		{
			bs.Read(timestamp);
			bs.Read(msgid);
		}

		switch(msgid)
		{
		case ID_CONNECTION_REQUEST_ACCEPTED:
			std::cout << "Connected to Server" << std::endl;
			break;

		case ID_NO_FREE_INCOMING_CONNECTIONS:
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			std::cout << "Lost Connection to Server" << std::endl;
			rakpeer_->DeallocatePacket(packet);
			return true;

		case ID_WELCOME:
			{
				unsigned int shipcount, id;
				float x_, y_;
				int type_;
				std::string temp;
				char chartemp[5];

				bs.Read(id);
				shipsList.at(0)->setID( id );	
				bs.Read(shipcount);

				for (unsigned int i = 0; i < shipcount; ++ i)
				{
					bs.Read(id);
					bs.Read(x_);
					bs.Read(y_);
					bs.Read(type_);
					std::cout << "New Ship pos" << x_ << " " << y_ << std::endl;
					Ship* ship = new Ship(type_, x_, y_ ); 
					temp = "Ship ";
					temp += _itoa(id, chartemp, 10);
					ship->SetName(temp.c_str());
					ship->setID( id );
					shipsList.push_back(ship);
				}

				SendInitialPosition();
			}
			break;

		case ID_NEWSHIP:
			{
				unsigned int id;
				bs.Read(id);

				if( id == shipsList.at(0)->GetID() )
				{
					// if it is me
					break;
				}
				else
				{
					float x_, y_;
					int type_;
					std::string temp;
					char chartemp[5];

					bs.Read( x_ );
					bs.Read( y_ );
					bs.Read( type_ );
					std::cout << "New Ship pos" << x_ << " " << y_ << std::endl;
					Ship* ship = new Ship(type_, x_, y_);
					temp = "Ship "; 
					temp += _itoa(id, chartemp, 10);
					ship->SetName(temp.c_str());
					ship->setID( id );
					shipsList.push_back(ship);
				}

			}
			break;

		case ID_LOSTSHIP:
			{
				unsigned int shipid;
				bs.Read(shipid);
				for (std::vector<Ship*>::iterator it = shipsList.begin(); it != shipsList.end(); ++it)
				{
					Ship* ship = static_cast<Ship*>(*it);
					if (ship->GetID() == shipid)
					{
						delete *it;
						shipsList.erase(it);
						break;
					}
				}
			}
			break;

		case ID_INITIALPOS:
			break;

		case ID_MOVEMENT:
			{
				unsigned int shipid;
				float temp;
				float x,y,w;
				bs.Read(shipid);

				for (std::vector<Ship*>::iterator it = shipsList.begin(); it != shipsList.end(); ++it)
				{
					Ship* ship = static_cast<Ship*>(*it);
					if (ship->GetID() == shipid)
					{
						// this portion needs to be changed for it to work
#ifdef INTERPOLATEMOVEMENT
						bs.Read(x);
						bs.Read(y);
						bs.Read(w);

						ship->SetServerLocation( x, y, w );

						bs.Read(temp);
						ship->SetServerVelocityX( temp );
						bs.Read(temp);
						ship->SetServerVelocityY( temp );
						bs.Read(temp);
						ship->SetAngularVelocity( temp );

						ship->DoInterpolateUpdate();
#else
						bs.Read(x);
						bs.Read(y);
						bs.Read(w);
						(*itr)->setLocation( x, y, w ); 

						// Lab 7 Task 1 : Read Extrapolation Data velocity x, velocity y & angular velocity
						bs.Read(temp);
						(*itr)->SetVelocityX( temp );
						bs.Read(temp);
						(*itr)->SetVelocityY( temp );
						bs.Read(temp);
						(*itr)->SetAngularVelocity( temp );
#endif

						break;
					}
				}
			}
			break;

		case ID_COLLIDE:
			{
				unsigned int shipid;
				float x, y;
				bs.Read(shipid);
				
				if( shipid == shipsList.at(0)->GetID() )
				{
					std::cout << "collided with someone!" << std::endl;
					bs.Read(x);
					bs.Read(y);
					shipsList.at(0)->SetX( x );
					shipsList.at(0)->SetY( y );
					bs.Read(x);
					bs.Read(y);
					shipsList.at(0)->SetVelocityX( x );
					shipsList.at(0)->SetVelocityY( y );
#ifdef INTERPOLATEMOVEMENT
					bs.Read(x);
					bs.Read(y);
					shipsList.at(0)->SetServerVelocityX( x );
					shipsList.at(0)->SetServerVelocityY( y );
#endif	
				}
			}
			break;


		// Lab 13 Task 10 : new cases to handle missile on application side
		case ID_NEWMISSILE:
		{
			float x, y, w;
			int id;

			bs.Read(id);
			bs.Read(x);
			bs.Read(y);
			bs.Read(w);

			missileList.push_back(new Laser("Laser.png", x, y, w, id));
		}
		break;

		case ID_UPDATEMISSILE:
		{
			float x, y, w;
			int id;
			char deleted;

			bs.Read(id);
			bs.Read(deleted);

			for (std::vector<Laser*>::iterator it = missileList.begin(); it != missileList.end(); ++it)
			{
				Laser* missile = static_cast<Laser*>(*it);
				if (missile->GetOwnerID() == id)
				{
					if (deleted == 1 || !missile->getActive())
					{
						delete *it;
						missileList.erase(it);
					}
					else
					{
						bs.Read(x);
						bs.Read(y);
						bs.Read(w);
						missile->UpdateLoc(x, y, w);
						bs.Read(x);
						missile->SetVelocityX(x);
						bs.Read(y);
						missile->SetVelocityY(y);
					}
					break;
				}
			}
		}
		break;

		default:
			std::cout << "Unhandled Message Identifier: " << msgid << std::endl;

		}
		rakpeer_->DeallocatePacket(packet);
	}


	sendData();

	return false;
}


/**
* Render Cycle
*
* Clear the screen and render all the ships
*/
void Application::Render()
{
	hge_->Gfx_BeginScene();
	hge_->Gfx_Clear(0);

	backgroundSprite->RenderEx(0, 0, 0);

	for (std::vector<Ship*>::iterator it = shipsList.begin(); it != shipsList.end(); ++it)
	{
		Ship* ship = static_cast<Ship*>(*it);
		ship->Render();
	}
	// Lab 13 Task 6 : Render the missile


	// Lab 13 Task 12 : Render network missiles
	for (std::vector<Laser*>::iterator it = missileList.begin(); it != missileList.end(); ++it)
	{
		Laser* missile = static_cast<Laser*>(*it);
		missile->Render();
	}

	for (std::vector<CAsteroid*>::iterator it = asteroidList.begin(); it != asteroidList.end(); ++it)
	{
		CAsteroid* asteroid = static_cast<CAsteroid*>(*it);
		asteroid->Render();
	}

	hge_->Gfx_EndScene();
}


/** 
* Main game loop
*
* Processes user input events
* Supposed to process network events
* Renders the ships
*
* This is a static function that is called by the graphics
* engine every frame, hence the need to loop through the
* global namespace to find itself.
*/
bool Application::Loop()
{
	Global::application->Render();
	return Global::application->Update();
}

/**
* Shuts down the graphics and network system
*/

void Application::Shutdown()
{
	hge_->System_Shutdown();
	hge_->Release();
}

/** 
* Kick starts the everything, called from main.
*/
void Application::Start()
{
	if (Init())
	{
		hge_->System_Start();
	}
}

bool Application::SendInitialPosition()
{
	RakNet::BitStream bs;
	unsigned char msgid = ID_INITIALPOS;
	bs.Write(msgid);
	bs.Write(shipsList.at(0)->GetX());
	bs.Write(shipsList.at(0)->GetY());
	bs.Write(shipsList.at(0)->GetType());

	std::cout << "Sending pos" << shipsList.at(0)->GetX() << " " << shipsList.at(0)->GetY() << std::endl;

	rakpeer_->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

	return true;
}

bool Application::checkCollisions(Ship* ship)
{
	for (std::vector<Ship*>::iterator it = shipsList.begin(); it != shipsList.end(); ++it)
	{
		Ship* ship2 = static_cast<Ship*>(*it);

		if(ship2 == ship ) continue;	//skip if it is the same ship

		if( ship->HasCollided(ship2))
		{
			if(ship2->CanCollide( RakNet::GetTime() ) &&  ship->CanCollide( RakNet::GetTime() ) )
			{
				std::cout << "collide!" << std::endl;

#ifdef INTERPOLATEMOVEMENT
			if( GetAbsoluteMag( ship->GetVelocityY() ) > GetAbsoluteMag(ship2->GetVelocityY() ) )
			{
				// this transfers vel to thisship
				ship2->SetVelocityY(ship2->GetVelocityY() + ship->GetVelocityY()/3 );
				ship->SetVelocityY( - ship->GetVelocityY() );

				ship2->SetServerVelocityY(ship2->GetServerVelocityY() + ship->GetServerVelocityY()/3 );
				ship->SetServerVelocityY( - ship->GetServerVelocityY() );
			}
			else
			{
				ship->SetVelocityY( ship->GetVelocityY() + ship2->GetVelocityY()/3 );
				ship2->SetVelocityY( -ship2->GetVelocityY()/2 );

				ship->SetServerVelocityY( ship->GetServerVelocityY() + ship2->GetServerVelocityY()/3 );
				ship2->SetServerVelocityY( -ship2->GetServerVelocityY()/2 );
			}
			
			if( GetAbsoluteMag( ship->GetVelocityX() ) > GetAbsoluteMag(ship2->GetVelocityX() ) )
			{
				// this transfers vel to thisship
				ship2->SetVelocityX(ship2->GetVelocityX() + ship->GetVelocityX()/3 );
				ship->SetVelocityX( - ship->GetVelocityX() );

				ship2->SetServerVelocityX(ship2->GetServerVelocityX() + ship->GetServerVelocityX()/3 );
				ship->SetServerVelocityX( - ship->GetServerVelocityX() );
			}
			else
			{
				// ship transfers vel to asteroid
				ship->SetVelocityX( ship->GetVelocityX() + ship2->GetVelocityX()/3 );
				ship2->SetVelocityX( -ship2->GetVelocityX()/2 );

				ship->SetServerVelocityX( ship->GetServerVelocityX() + ship2->GetServerVelocityX()/3 );
				ship2->SetServerVelocityX( -ship2->GetServerVelocityX()/2 );
			}

				ship->SetPreviousLocation();
#else
			if( GetAbsoluteMag( ship->GetVelocityY() ) > GetAbsoluteMag( (*thisship)->GetVelocityY() ) )
			{
				// this transfers vel to thisship
				(*thisship)->SetVelocityY( (*thisship)->GetVelocityY() + ship->GetVelocityY()/3 );
				ship->SetVelocityY( - ship->GetVelocityY() );
			}
			else
			{
				ship->SetVelocityY( ship->GetVelocityY() + (*thisship)->GetVelocityY()/3 ); 
				(*thisship)->SetVelocityY( -(*thisship)->GetVelocityY()/2 );
			}
			
			if( GetAbsoluteMag( ship->GetVelocityX() ) > GetAbsoluteMag( (*thisship)->GetVelocityX() ) )
			{
				// this transfers vel to thisship
				(*thisship)->SetVelocityX( (*thisship)->GetVelocityX() + ship->GetVelocityX()/3 );
				ship->SetVelocityX( - ship->GetVelocityX() );
			}
			else
			{
				// ship transfers vel to asteroid
				ship->SetVelocityX( ship->GetVelocityX() + (*thisship)->GetVelocityX()/3 ); 
				(*thisship)->SetVelocityX( -(*thisship)->GetVelocityX()/2 );
			}


//				ship->SetVelocityY( -ship->GetVelocityY() );
//				ship->SetVelocityX( -ship->GetVelocityX() );
			
				ship->SetPreviousLocation();
#endif
				SendCollision(ship2);

				return true;
			}
				
		}

	}
	
	return false;
}

void Application::SendCollision( Ship* ship )
{
	RakNet::BitStream bs;
	unsigned char msgid = ID_COLLIDE;
	bs.Write( msgid );
	bs.Write( ship->GetID() );
	bs.Write( ship->GetX() );
	bs.Write( ship->GetY() );
	bs.Write( ship->GetVelocityX() );
	bs.Write( ship->GetVelocityY() );
#ifdef INTERPOLATEMOVEMENT
	bs.Write( ship->GetServerVelocityX() );
	bs.Write( ship->GetServerVelocityY() );
#endif

	rakpeer_->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

}

void Application::CreateMissile(float x, float y, float w, int id)
{
#ifdef NETWORKMISSILE
	// Lab 13 Task 9b : Implement networked version of createmissile
	RakNet::BitStream bs;
	unsigned char msgid;
	unsigned char deleted = 0;

	Laser* missile = new Laser("Laser.png", x, y, w, id);
	missileList.push_back(missile);

	msgid = ID_NEWMISSILE;
	bs.Reset();
	bs.Write(msgid);
	bs.Write(id);
	bs.Write(x);
	bs.Write(y);
	bs.Write(w);
	rakpeer_->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
#else
	// Lab 13 Task 3 : Implement local version missile creation

#endif
}

void Application::CreateAsteroid(float x, float y, float w, bool smallAsteroid)
{
	RakNet::BitStream bs;
	bs.Reset();
	unsigned char msgid;
	unsigned char smallSize;
	CAsteroid* asteroid;

	/*if (!smallAsteroid)
		asteroid = new CAsteroid(x, y, w);
	else
		asteroid = new CAsteroid(x, y, w, true);*/
	asteroid = new CAsteroid(x, y, w);
	asteroidList.push_back(asteroid);

	msgid = ID_NEW_ASTEROID;
	if (!smallAsteroid)
		smallSize = 1;
	else
		smallSize = 0;
	bs.Write(msgid);
	bs.Write(smallSize);
	bs.Write(x);
	bs.Write(y);
	bs.Write(w);
	rakpeer_->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
}


void Application::playerControl(const float dt)
{
	shipsList.at(0)->SetAngularVelocity(0.0f);

	if (hge_->Input_GetKeyState(HGEK_LEFT))
	{
		shipsList.at(0)->SetAngularVelocity(shipsList.at(0)->GetAngularVelocity() - DEFAULT_ANGULAR_VELOCITY);
	}

	if (hge_->Input_GetKeyState(HGEK_RIGHT))
	{
		shipsList.at(0)->SetAngularVelocity(shipsList.at(0)->GetAngularVelocity() + DEFAULT_ANGULAR_VELOCITY);
	}

	if (hge_->Input_GetKeyState(HGEK_UP))
	{
		shipsList.at(0)->Accelerate(DEFAULT_ACCELERATION, dt);
	}

	if (hge_->Input_GetKeyState(HGEK_DOWN))
	{
		shipsList.at(0)->Accelerate(-DEFAULT_ACCELERATION, dt);
	}

	// Lab 13 Task 4 : Add a key to shoot missiles
	static bool keydownEnter = false;
	static float currentTime = 0, timeLimit = 0.5;

	if (currentTime < timeLimit)
	{
		currentTime += dt;
	}
	else
	{
		if (hge_->Input_GetKeyState(HGEK_ENTER))
		{
			CreateMissile(shipsList.at(0)->GetX(), shipsList.at(0)->GetY(), shipsList.at(0)->GetW(), shipsList.at(0)->GetID());
			currentTime = 0.f;
		}
	}

	static bool keydownEnter_A = false;
	static float currentTime_A = 0, timeLimit_A = 0.5;
	if (currentTime_A < timeLimit_A)
	{
		currentTime_A += dt;
	}
	else
	{
		if (hge_->Input_GetKeyState(HGEK_A))
		{
			float x = Math::RandFloatMinMax(0, 800);
			float y = Math::RandFloatMinMax(0, 600);
			float w = Math::RandFloatMinMax(0, 360);
			CreateAsteroid(x, y, w);
			currentTime_A = 0.f;
		}
	}
}

void Application::sendData()
{
	if (RakNet::GetTime() - timer_ > 1000)
	{
		timer_ = RakNet::GetTime();
		RakNet::BitStream bs;
		unsigned char msgid = ID_MOVEMENT;
		bs.Write(msgid);

#ifdef INTERPOLATEMOVEMENT
		bs.Write(shipsList.at(0)->GetID());
		bs.Write(shipsList.at(0)->GetServerX());
		bs.Write(shipsList.at(0)->GetServerY());
		bs.Write(shipsList.at(0)->GetServerW());
		bs.Write(shipsList.at(0)->GetServerVelocityX());
		bs.Write(shipsList.at(0)->GetServerVelocityY());
		bs.Write(shipsList.at(0)->GetAngularVelocity());

#else
		bs2.Write(ships_.at(0)->GetID());
		bs2.Write(ships_.at(0)->GetX());
		bs2.Write(ships_.at(0)->GetY());
		bs2.Write(ships_.at(0)->GetW());
		// Lab 7 Task 1 : Add Extrapolation Data velocity x, velocity y & angular velocity
		bs2.Write(ships_.at(0)->GetVelocityX());
		bs2.Write(ships_.at(0)->GetVelocityY());
		bs2.Write(ships_.at(0)->GetAngularVelocity());
#endif

		rakpeer_->Send(&bs, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);


		// Lab 13 Task 11 : send missile update 
		for (std::vector<Laser*>::iterator it = missileList.begin(); it != missileList.end(); ++it)
		{
			Laser* missile = static_cast<Laser*>(*it);
			RakNet::BitStream bs2;
			unsigned char msgid2 = ID_UPDATEMISSILE;
			unsigned char deleted = 0;
			bs2.Write(msgid2);
			bs2.Write(missile->GetOwnerID());
			bs2.Write(deleted);
			bs2.Write(missile->GetX());
			bs2.Write(missile->GetY());
			bs2.Write(missile->GetW());
			bs2.Write(missile->GetVelocityX());
			bs2.Write(missile->GetVelocityY());
			rakpeer_->Send(&bs2, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		}

		for (std::vector<CAsteroid*>::iterator it = asteroidList.begin(); it != asteroidList.end(); ++it)
		{
			CAsteroid* asteroid = static_cast<CAsteroid*>(*it);
			RakNet::BitStream bs2;
			unsigned char msgid2 = ID_UPDATE_ASTEROID;

			bs2.Write(msgid2);
			bs2.Write(asteroid->GetX());
			bs2.Write(asteroid->GetY());
			bs2.Write(asteroid->GetW());
			bs2.Write(asteroid->GetVelocityX());
			bs2.Write(asteroid->GetVelocityY());
			rakpeer_->Send(&bs2, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		}

		msgid = ID_NEW_ASTEROID;
		bs.Reset();
		bs.Write(msgid);
		/*bs.Write(asteroidList.size());*/
		rakpeer_->Send(&bs, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		/*if (asteroidList.size() < 10 && shipsList.at(0)->GetID() == 1)
		{
			unsigned char msgid2 = ID_SPAWNASTEROID;
			RakNet::BitStream bs2;
			bs2.Write(msgid2);
			rakpeer_->Send(&bs2, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		}*/
	}
}