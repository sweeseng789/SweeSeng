#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "ship.h"
#include "Laser.h"
#include "Asteroid.h"
#include <vector>

class HGE;
class RakPeerInterface;

//! The default angular velocity of the ship when it is in motion
static const float DEFAULT_ANGULAR_VELOCITY = 3.0f; 
//! The default acceleration of the ship when powered
static const float DEFAULT_ACCELERATION = 50.0f;

/**
* The application class is the main body of the program. It will
* create an instance of the graphics engine and execute the
* Update/Render cycle.
*
*/

class Application
{
	HGE* hge_; //!< Instance of the internal graphics engine
	//typedef std::vector<Ship*> ShipList;  //!< A list of ships
	std::vector<Ship*> shipsList; //!< List of all the ships in the universe
	std::vector<Laser*> missileList;
	std::vector<CAsteroid*> asteroidList;
	RakPeerInterface* rakpeer_;
	unsigned int timer_;

	std::auto_ptr<hgeSprite> backgroundSprite;
	
	// Lab 13 Task 1 : add variables for local missle


	// Lab 13 Task 8 : add variables to handle networked missiles


	bool Init();
	static bool Loop();
	void Shutdown();
	bool checkCollisions(Ship* ship);
	void ProcessWelcomePackage();
	bool SendInitialPosition();
	void sendData();

	// Lab 13
	void CreateMissile( float x, float y, float w, int id );
	bool RemoveMissile( float x, float y, float w, int id );

	void CreateAsteroid(float x, float y, float w, bool smallAsteroid = false);

	void SendCollision( Ship* ship );

	void playerControl(const float dt);

public:
	Application();
	~Application() throw();

	void Start();
	bool Update();
	void Render();
};

#endif