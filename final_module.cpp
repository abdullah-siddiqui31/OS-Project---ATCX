/***********************************
M. DANIYAL            23I-0579  CS-F
M. ABDULLAH SIDDIQUI  23I-0617  CS-F
************************************/

// Direction : North, East, West, South
// FlightType : International_Arrival, Domestic_Arrival, International_Departure, Domestic_Departure
// AircraftType : Commercial, Military, Cargo, Medical
// Phases : Holding, Approaching, Landing, Taxi, AtGate, Climb, TakeoffRoll, Departure


#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <SFML/Graphics.hpp>


using namespace std;
using namespace sf;




class Restrictions_aircrafts_Flight
{
public:
	string airline_name;
	int max_aircrafts;
	int max_flights;
};

const int number_of_airlines = 6;
Restrictions_aircrafts_Flight airlines[number_of_airlines] =
	{{"PIA", 6, 4}, {"AirBlue", 4, 4}, {"FedEx", 3, 2}, {"Pakistan Airforce", 2, 1}, {"Blue Dart", 2, 2}, {"AghaKhan Air Ambulance", 2, 1}};


class Flight
{
public:
	string name;
	string runaway;
	string phase;
	string direction;
	string flight_type;
	string aircraft_type;
	string airline;
	bool isEmergency;
	bool avnStatus;
	bool isFaulty;
	bool assined;
	int emergency_probability;
	int speed;
	int time_minute, time_second;
	int changing_phase_time;
	int priority;
	int wait_in_secs;
	sem_t w;

	Flight()
	{
		sem_init(&w, 0, 0);
	}

	void Add(string n, bool status, bool faulty,
			 string p, string d, string ft, string at, int m, int s)
	{
		time_minute = m;
		time_second = s;
		name = n;
		avnStatus = status;
		isFaulty = faulty;
		phase = p;
		direction = d;
		flight_type = ft;
		aircraft_type = at;
		assined = 0;
		changing_phase_time = 0;

		if (d == "North" || d == "South")
			runaway = "RWY-A";
		else if (d == "West" || d == "East")
			runaway = "RWY-B";

		// Setting emergency bool
		if (aircraft_type == "Military" || aircraft_type == "Medical" || aircraft_type == "Cargo")
		{
			isEmergency = true;
			runaway = "RWY-C";
			if (aircraft_type == "Medical")
				priority = 1;
			else if (aircraft_type == "Military")
				priority = 2;
			else if (aircraft_type == "Cargo")
				priority = 3;
		}
		else
		{
			isEmergency = false;
			priority = 4;
		}
		// setting emergency probability
		if (direction == "North" && flight_type == "International_Arrival")
		{
			emergency_probability = 10;
		}
		else if (direction == "South" && flight_type == "Domestic_Arrival")
			emergency_probability = 5;
		else if (direction == "East" && flight_type == "International_Departure")
			emergency_probability = 15;
		else if (direction == "West" && flight_type == "Domestic_Departure")
			emergency_probability = 20;
		else
			emergency_probability = 0;

		// Setting speed
		if (flight_type == "International_Arrival" || flight_type == "Domestic_Arrival")
		{
			speed = 600;
			phase = "Holding";
		}
		else if (flight_type == "International_Departure" || flight_type == "Domestic_Departure")
		{
			speed = 0;
			phase = "AtGate";
		}
	}
};

class Airline
{
public:
	string name;
	string type;
	int aircrafts;
	int Maxflights, curr_flights;
	Flight **flight;
	int x = 0;

	Airline(string n, string t)
	{
		for (int i = 0; i < number_of_airlines; i++)
		{
			if (airlines[i].airline_name == n)
			{
				aircrafts = airlines[i].max_aircrafts;
				Maxflights = airlines[i].max_flights;
			}
		}
		curr_flights = 0;
		name = n;
		type = t;
		flight = new Flight *[Maxflights];
	}

	void AddFlight(Flight *f)
	{
		if (x < Maxflights)
		{
			flight[x] = f;
			x++;
			curr_flights = x;
		}
		else
		{
			cout << "Cannot add more flights for this Airline" << endl;
		}
		return;
	}
};

class Runways
{
public:
	string name;
	string direction;
	Flight *f;

	Runways()
	{
		f = NULL;
	}
};


int return_airline_index(string str)
{
    if(str == "PIA")
        return 0;
    if(str ==  "AirBlue")
        return 1;
    if(str == "FedEx")
        return 2;
    if(str == "Pakistan Airforce")
        return 3;
    if(str == "Blue Dart")
        return 4;
    if(str == "AghaKhan Air Ambulance")
        return 5;
    return -1;
}


int fd1[2], fd2[2], fd3[2], fd4[2], fd5[2], fd6[2];
float RealTime = 5 * 60, CurrentTime = 0;
unordered_map<string, int> phaseTime;

bool status = 0;
sem_t sem, sem2;

Runways RWY[3];


int turn = 2;
int min = 0, sec = 0;

sem_t s2, s3, s4;

Sprite planes_sprite[3], runway_sprite[3];
Text text[3];
unordered_map<string, int> Y_text;

queue<Flight *> ra, rb, rc;
sem_t s1, s1b, s1c;


vector<Flight *> simulating_flights;

/////////////////////////////////////////////   SFML STUFF

int returnX(string str)
{
	if (str == "RWY-A")
		return 50;
	if (str == "RWY-B")
		return 250;
	if (str == "RWY-C")
		return 450;

	return -1;
}


void initialize_YText()
{
	Y_text["Holding"] = 20;
	Y_text["Departure"] = 20;
	Y_text["Approaching"] = 100;
	Y_text["Climb"] = 100;
	Y_text["Landing"] = 200;
	Y_text["TakeoffRoll"] = 200;
	Y_text["Taxi"] = 300;
	Y_text["AtGate"] = 400;
}

void SFML_Stuff(Runways &rnway)
{
	if (rnway.name == "RWY-A")
	{
		planes_sprite[0].setPosition(returnX("RWY-A"), Y_text[rnway.f->phase]);
		text[0].setString(rnway.f->phase);
		text[0].setPosition(returnX("RWY-A") + 50, Y_text[rnway.f->phase] + 50);
	}
	else if (rnway.name == "RWY-B")
	{
		planes_sprite[1].setPosition(returnX("RWY-B"), Y_text[rnway.f->phase]);
		text[1].setString(rnway.f->phase);
		text[1].setPosition(returnX("RWY-B") + 50, Y_text[rnway.f->phase] + 50);
	}
	else if (rnway.name == "RWY-C")
	{
		planes_sprite[2].setPosition(returnX("RWY-C"), Y_text[rnway.f->phase]);
		text[2].setString(rnway.f->phase);
		text[2].setPosition(returnX("RWY-C") + 50, Y_text[rnway.f->phase] + 50);
	}
}



void initiatePhaseTime()
{
	phaseTime["Holding"] = 2;
	phaseTime["Approaching"] = 2;
	phaseTime["Landing"] = 2;
	phaseTime["Taxi"] = 2;
	phaseTime["AtGate"] = 2;
	phaseTime["TakeoffRoll"] = 2;
	phaseTime["Climb"] = 2;
	phaseTime["Departure"] = 2;
}


string which_airline(int i)
{
	if (i == 1)
		return "PIA";
	else if (i == 2)
		return "FedEx";
	else if (i == 3)
		return "Pakistan Airforce";
	else if (i == 4)
		return "AghaKhan Air Ambulance";
	else if (i == 5)
		return "Blue Dart";
	else if (i == 6)
		return "Air Blue";
	return "";
}

string which_flight(int i)
{
	if (i == 1)
		return "International_Arrival";
	else if (i == 2)
		return "Domestic_Arrival";
	else if (i == 3)
		return "Domestic_Departure";
	else if (i == 4)
		return "International_Departure";
	return "";
}

string which_aircraft(int i)
{
	if (i == 1)
		return "Commercial";
	else if (i == 2)
		return "Cargo";
	else if (i == 3)
		return "Military";
	else if (i == 4)
		return "Medical";
	return "";
}

string direction(int i)
{
	if (i == 1)
		return "North";
	else if (i == 2)
		return "South";
	else if (i == 3)
		return "East";
	else if (i == 4)
		return "West";
	return "";
}




void *timer(void *a)
{
	while (RealTime >= 0)
	{
		while (turn == 1)
			;
		turn = 2;
		cout << "TIME -> " << ::min << ":" << ::sec << endl;
		sleep(1);
		CurrentTime++;
		::sec++;
		if (::sec >= 60)
		{
			::sec = 0;
			::min++;
		}
		RealTime--;
		turn = 1;
	}
	return NULL;
}



void *Radar(void *arg)
{
	while (RealTime >= 0)
	{
		while (turn == 2);
		turn = 1;
		for (int i = 0; i < 3; i++)
		{
			if (RWY[i].f == NULL)
				continue;
			Flight &flight = *(RWY[i].f);
			// RWY

			if (flight.flight_type == "International_Arrival" || flight.flight_type == "Domestic_Arrival")
			{
				//------------------------(AVN Control)---------------------
				if (
					(((flight.phase == "Holding") && (flight.speed > 600 || flight.speed < 400)) ||
					 ((flight.phase == "Approaching") && (flight.speed > 290 || flight.speed < 240)) ||
					 ((flight.phase == "Landing") && (flight.speed > 240 || flight.speed < 30)) ||
					 ((flight.phase == "Taxi") && (flight.speed > 240 || flight.speed < 30)) ||
					 ((flight.phase == "AtGate") && (flight.speed > 5 || flight.speed < 0))) &&
					flight.avnStatus == false)
				{
					sem_wait(&sem);
					flight.avnStatus = true;
					cout << flight.name << " You violated the limit! " << endl;
					sem_post(&sem2);
					sem_wait(&sem2);
					for(int z=0; z<3; z++){
					if (z!=1 && RWY[z].f!=NULL && RWY[z].f->avnStatus == 1)
					{
						int w = rand() % 1200 + 1;
						string result = "";
						//AVN REPORT SERIALIZATION
						result += to_string(w) + "|" + RWY[z].f->name + "|";
                                                result += RWY[z].f->airline + "|" + RWY[z].f->aircraft_type + "|";
                                                result += to_string(RWY[z].f->speed) + "|" + "unpaid|" ;
						char message[5000];
						write(fd1[1], result.c_str(), strlen(result.c_str())+1);
						read(fd6[0], message, sizeof(message));
						string ta(message);
						if(ta=="Y")
						  cout << "Payment Cleared" << endl;
						else
						  cout<<"Fine Not Paid"<<endl;
					}
					}
					sem_post(&sem);
				}
				//------------------------(Phase Control)---------------------
				if (flight.phase == "Holding" && flight.changing_phase_time >= phaseTime["Holding"])
				{
					cout << flight.name << " Shifting To Approaching Phase  " << endl;
					flight.avnStatus = false;
					flight.phase = "Approaching";
					flight.changing_phase_time = 0;
					flight.speed = rand() % 310 + 220;
				}
				else if (flight.phase == "Approaching" && flight.changing_phase_time >= phaseTime["Approaching"])
				{
					cout << flight.name << " Shifting To Landing Phase " << endl;
					flight.avnStatus = false;
					flight.phase = "Landing";
					flight.changing_phase_time = 0;
					flight.speed = rand() % 240 + 25;
				}
				else if (flight.phase == "Landing" && flight.changing_phase_time >= phaseTime["Landing"])
				{
					cout << flight.name << " Shifting To Taxi Phase " << endl;
					flight.avnStatus = false;
					flight.phase = "Taxi";
					flight.changing_phase_time = 0;
					flight.speed = rand() % 35 + 15;
				}
				else if (flight.phase == "Taxi" && flight.changing_phase_time >= phaseTime["Taxi"])
				{
					cout << flight.name << " Moving Towards Gate " << endl;
					flight.avnStatus = false;
					flight.phase = "AtGate";
					flight.changing_phase_time = 0;
					flight.speed = rand() % 15;
				}
				else if (flight.phase == "AtGate" && flight.changing_phase_time >= phaseTime["AtGate"])
				{
					cout << flight.name << " FLIGHT SUCCESSFULLY ARRIVED Runway : " << flight.runaway << endl;
					string temp = RWY[i].f->runaway;
					RWY[i].f = NULL;
					flight.changing_phase_time = 0;
					flight.assined = 1;
					if (temp == "RWY-A")
						sem_post(&s2);
					else if (temp == "RWY-B")
						sem_post(&s3);
					else if (temp == "RWY-C")
						sem_post(&s4);
				}
				else
				{
					flight.changing_phase_time += 1;
				}
			}
			else
			{
				//------------------------(AVN Control)---------------------
				if (
					(((flight.phase == "Departed") && (flight.speed > 900 || flight.speed < 800)) ||
					 ((flight.phase == "Climb") && (flight.speed > 463 || flight.speed < 250)) ||
					 ((flight.phase == "TakeoffRoll") && (flight.speed > 290 || flight.speed < 0)) ||
					 ((flight.phase == "Taxi") && (flight.speed > 30 || flight.speed < 15)) ||
					 ((flight.phase == "AtGate") && (flight.speed > 5 || flight.speed < 0))) &&
					flight.avnStatus == false)
				{
					flight.avnStatus = true;
					sem_wait(&sem);
					cout << flight.name << "You violated the limit! " << endl;
					sem_post(&sem2);
					sem_wait(&sem2);
					for(int z=0; z<3; z++){
					if (z!=0 && RWY[z].f && RWY[z].f->avnStatus == 1)
					{
						int w = rand() % 1200 + 1;
						string result = "";
						//AVN REPORT SERIALIZATION
						result += to_string(w) + "|" + RWY[z].f->name + "|";
                                                result += RWY[z].f->airline + "|" + RWY[z].f->aircraft_type + "|";
                                                result += to_string(RWY[z].f->speed) + "|" + "unpaid|";
						char message[5000];
						write(fd1[1], result.c_str(), strlen(result.c_str())+1);
						read(fd6[0], message, sizeof(message));
						string ta(message);
						if(ta=="Y")
						  cout << "Payment Cleared" << endl;
						else
						  cout<<"Fine Not Paid"<<endl;
					}}
					sem_post(&sem);
				}
				//------------------------(Phase Control)---------------------
				if (flight.phase == "AtGate" && flight.changing_phase_time >= phaseTime["AtGate"])
				{
					cout << flight.name << " Shifting To Taxi Phase " << endl;
					flight.avnStatus = false;
					flight.phase = "Taxi";
					flight.changing_phase_time = 0;
					flight.speed = rand() % 35 + 15;
				}
				else if (flight.phase == "Taxi" && flight.changing_phase_time >= phaseTime["Taxi"])
				{
					cout << flight.name << " Shifting To Take Off Roll Phase " << endl;
					flight.avnStatus = false;
					flight.phase = "TakeoffRoll";
					flight.changing_phase_time = 0;
					flight.speed = rand() % 295;
				}
				else if (flight.phase == "TakeoffRoll" && flight.changing_phase_time >= phaseTime["TakeoffRoll"])
				{
					cout << flight.name << " Shifting To Climb Phase " << endl;
					flight.avnStatus = false;
					flight.phase = "Climb";
					flight.changing_phase_time = 0;
					flight.speed = rand() % 470 + 245;
				}
				else if (flight.phase == "Climb" && flight.changing_phase_time >= phaseTime["Climb"])
				{
					cout << flight.name << " Flying Towards Sky! " << endl;
					flight.avnStatus = false;
					flight.phase = "Departed";
					flight.changing_phase_time = 0;
					flight.speed = rand() % 920 + 790;
				}
				else if (flight.phase == "Departed" && flight.changing_phase_time >= phaseTime["Departure"])
				{
					cout << flight.name << " FLIGHT DEPARTED SUCCESSFULLY! Runway : " << flight.runaway << endl;
					string temp = RWY[i].f->runaway;
					RWY[i].f = NULL;
					flight.changing_phase_time = 0;
					flight.assined = 1;
					if (temp == "RWY-A")
						sem_post(&s2);
					else if (temp == "RWY-B")
						sem_post(&s3);
					else if (temp == "RWY-C")
						sem_post(&s4);
				}
				else
				{
					flight.changing_phase_time += 1;
				}
			}
		}
		turn = 2;
	}
	return NULL;
}


void *RunwayWaitQueue(void *a)
{
	Flight *f1 = (Flight *)a;

	if (f1->runaway == "RWY-A")
	{
		sem_wait(&s1);
		ra.push(f1);
		sem_post(&s1);
	}
	else if (f1->runaway == "RWY-B")
	{
		sem_wait(&s1b);
		rb.push(f1);
		sem_post(&s1b);
	}
	else if (f1->runaway == "RWY-C")
	{
		sem_wait(&s1c);
		rc.push(f1);
		sem_post(&s1c);
	}

	sem_wait(&f1->w);

	return NULL;
}

void *dispatcherRWY_A(void *a)
{

	do
	{
		if (!ra.empty())
		{
			planes_sprite[0].setPosition(-100, -100);
			ra.front()->wait_in_secs = CurrentTime - (ra.front()->time_minute * 60 + ra.front()->time_second);
			cout << ra.front()->name << "  " << ra.front()->wait_in_secs << endl;
			RWY[0].f = ra.front();
			sem_wait(&s2);
			sem_post(&ra.front()->w);
			ra.pop();
		}
	} while (RealTime >= 0);

	return NULL;
}

void *dispatcherRWY_B(void *a)
{

	do
	{
		if (!rb.empty())
		{
			planes_sprite[1].setPosition(-100, -100);
			rb.front()->wait_in_secs = CurrentTime - (rb.front()->time_minute * 60 + rb.front()->time_second);
			cout << rb.front()->name << "  " << rb.front()->wait_in_secs << endl;
			RWY[1].f = rb.front();
			sem_wait(&s3);
			sem_post(&rb.front()->w);
			rb.pop();
		}
	} while (RealTime >= 0);

	return NULL;
}

void *dispatcherRWY_C(void *a)
{

	do
	{
		if (!rc.empty())
		{
			planes_sprite[2].setPosition(-100, -100);
			rc.front()->wait_in_secs = CurrentTime - (rc.front()->time_minute * 60 + rc.front()->time_second);
			cout << rc.front()->name << "  " << rc.front()->wait_in_secs << endl;
			RWY[2].f = rc.front();
			sem_wait(&s4);
			sem_post(&rc.front()->w);
			rc.pop();
		}
	} while (RealTime >= 0);

	return NULL;
}

void *airline_portal(void* arg)
{

    cout << "Airline portal thread running" << std::endl;

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void setupRadar(
    CircleShape& radarBase,
    vector<CircleShape>& radarCircles,
    RectangleShape& sweep,
    const float radius,
    const Vector2f& center
) {
    // Radar base
    radarBase.setRadius(radius);
    radarBase.setFillColor(Color::Black);
    radarBase.setOutlineColor(Color::Green);
    radarBase.setOutlineThickness(2);
    radarBase.setOrigin(radius, radius);
    radarBase.setPosition(center);

    // Radar rings
    radarCircles.clear();
    for (int i = 1; i <= 4; ++i) {
        CircleShape ring(radius * i / 4.0f);
        ring.setFillColor(Color::Transparent);
        ring.setOutlineColor(Color(0, 255, 0, 100));
        ring.setOutlineThickness(1);
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setPosition(center);
        radarCircles.push_back(ring);
    }

    // Sweep line
    sweep.setSize(Vector2f(radius, 2.0f));
    sweep.setOrigin(0, 1);
    sweep.setPosition(center);
    sweep.setFillColor(Color::Green);
}



//////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
	int arrivals = 0, departures = 0;
	srand(time(0));

	// Creating Airlines
	Airline arr[6] = {{"PIA", "Commercial"}, {"FedEx", "Cargo"}, {"Pakistan Airforce", "Military"}, {"AghaKhan Air Ambulance", "Medical"}, {"Blue Dart", "Cargo"}, {"Air Blue", "Commercial"}};

	// Flights Data Input
	int ch = 1, counter = 1;
	cout << "Airlines : 1. PIA\n2. FedEx\n3. Pakistan Airforce\n4. AghaKhan Air Ambulance\n5. Blue Dart\n6. Air Blue\n"
		 << endl;
	cout << "Aircraft Type : 1. Commercial\n2. Cargo\n3. Military\n4. Medical\n"
		 << endl;
	cout << "Directions : 1. North\n2. South\n3. East\n4. West\n"
		 << endl;
	cout << "Flight Type : 1. International_Arrival\n2. Domestic_Arrival\n3. Domestic_Departure\n4. International_Departure\n"
		 << endl
		 << endl;
	cout << "-----:( Schedule Flights ):-----" << endl;
	cout << "SR.   Airline      Flight Number     Direction         Flight Type         Aircraft Type    Minute    Seconds    1/0" << endl;
	do
	{
		string a, b, c, d, e;
		int j = 0, i = 0, k = 0, l = 0, p = 0, q = 0;
		cout << counter << "   ";
		cin >> k >> b >> q >> l >> p >> j >> i;

		/*
		
		b = "F1";
		k  = q = l = p = 1;
		j = 0;
		i = 3;
		*/
		a = which_airline(k);
		d = which_flight(l);
		e = which_aircraft(p);
		c = direction(q);
		Flight *f = new Flight();
		if (d == "International_Arrival" || d == "Domestic_Arrival")
		{
			f->Add(b, false, false, "Holding", c, d, e, j, i);
		}
		else
		{
			f->Add(b, false, false, "AtGate", c, d, e, j, i);
		}
		if (a == "PIA")
			arr[0].AddFlight(f);
		else if (a == "Air Blue")
			arr[5].AddFlight(f);
		else if (a == "Blue Dart")
			arr[4].AddFlight(f);
		else if (a == "AghaKhan Air Ambulance")
			arr[3].AddFlight(f);
		else if (a == "Pakistan Airforce")
			arr[2].AddFlight(f);
		else if (a == "FedEx")
			arr[1].AddFlight(f);

		counter++;

		cin >> ch;
	} while (ch != 0);

	sf::Clock clock; // SFML clock for timing
	float lastUpdateTime = 0.0f;

	// PRIORITY SCHEDULING + FCFS SCHEDULING
	queue<Flight *> arrival;
	queue<Flight *> departure;

	for (int i = 0; i < number_of_airlines; i++)
	{
		if (arr[i].curr_flights > 0)
		{
			for (int j = 0; j < arr[i].curr_flights; j++)
			{
				simulating_flights.push_back(arr[i].flight[j]);
			}
		}
	}

	// FCFS
	for (int i = 0; i < simulating_flights.size(); i++)
	{
		for (int j = 0; j < simulating_flights.size() - 1; j++)
		{
			if (simulating_flights[j]->time_minute > simulating_flights[j + 1]->time_minute ||
				(simulating_flights[j]->time_minute == simulating_flights[j + 1]->time_minute &&
				 simulating_flights[j]->time_second > simulating_flights[j + 1]->time_second))
			{
				Flight *temp = simulating_flights[j];
				simulating_flights[j] = simulating_flights[j + 1];
				simulating_flights[j + 1] = temp;
			}
		}
	}

	// Prioritizing (Emergency > VIP > Cargo > Commercial)
	for (int j = 0; j < simulating_flights.size() - 1; j++)
	{
		if (simulating_flights[j]->time_minute == simulating_flights[j + 1]->time_minute &&
			simulating_flights[j]->time_second == simulating_flights[j + 1]->time_second)
		{
			if (simulating_flights[j]->priority > simulating_flights[j + 1]->priority)
			{
				Flight *temp = simulating_flights[j];
				simulating_flights[j] = simulating_flights[j + 1];
				simulating_flights[j + 1] = temp;
			}
		}
	}

	// Arrivals & Departures Queue
	for (int i = 0; i < simulating_flights.size(); i++)
	{
		if (simulating_flights[i]->flight_type == "International_Arrival" || simulating_flights[i]->flight_type == "Domestic_Arrival")
		{
			arrival.push(simulating_flights[i]);
			arrivals++;
		}
		else
		{
			departure.push(simulating_flights[i]);
			departures++;
		}
	}

	initiatePhaseTime();
	sem_init(&s1, 0, 1);
	sem_init(&s1b, 0, 1);
	sem_init(&s1c, 0, 1);
	sem_init(&s2, 0, 0);
	sem_init(&s3, 0, 0);
	sem_init(&s4, 0, 0);

	// 3 Runways, Each with a flight queue
	RWY[0].name = "RWY-A";
	RWY[1].name = "RWY-B";
	RWY[2].name = "RWY-C";

	pthread_t r, t, *departure_flights, *arrival_flights, dispatchA, dispatchB, dispatchC, airline_portal_thread;
	pthread_create(&r, NULL, Radar, NULL);
	pthread_detach(r);
	pthread_create(&t, NULL, timer, NULL);
	pthread_detach(t);
	pthread_create(&dispatchA, NULL, dispatcherRWY_A, NULL);
	pthread_detach(dispatchA);
	pthread_create(&dispatchB, NULL, dispatcherRWY_B, NULL);
	pthread_detach(dispatchB);
	pthread_create(&dispatchC, NULL, dispatcherRWY_C, NULL);
	pthread_detach(dispatchC);


	if (departures > 0)
		departure_flights = new pthread_t[departures];
	if (arrivals > 0)
		arrival_flights = new pthread_t[arrivals];
	int a_count = 0, d_count = 0;


	// Create a window
	sf::RenderWindow window(sf::VideoMode(1180, 900), "Runway Display");

	sf::Font font;
	if (!font.loadFromFile("imgs/Poppins-Black.ttf"))
	{
		return -1; 
	}

	sf::Texture planeTexture;
	planeTexture.loadFromFile("imgs/plane_white.png");

	sf::Texture runwayTexture;
	runwayTexture.loadFromFile("imgs/runway.png");

	sf::Texture background_texture;
	background_texture.loadFromFile("imgs/bg.jpg");
	Sprite background;
	background.setTexture(background_texture);
	background.setPosition(0, 0);
	//background.setScale(1.5, 1.5);

	sf::Texture atc_texture;
	atc_texture.loadFromFile("imgs/atc.png");
	Sprite Atc_Sprite;
	Atc_Sprite.setTexture(atc_texture);
	Atc_Sprite.setPosition(850, 300);
	//Atc_Sprite.setScale(1, 1);

	sf::Texture ufoTexture;
	ufoTexture.loadFromFile("imgs/ufo_blue.png");

	for (int i = 0; i < 3; ++i)
	{
		text[i].setFont(font);
		text[i].setCharacterSize(24);
		text[i].setFillColor(sf::Color::White);

		planes_sprite[i].setScale(0.5, 0.5);
		planes_sprite[i].setPosition(-100, -100);

		runway_sprite[i].setTexture(runwayTexture);
		runway_sprite[i].setScale(0.3, 0.6);
	}

	planes_sprite[0].setTexture(planeTexture);
	planes_sprite[1].setTexture(planeTexture);
	planes_sprite[2].setTexture(ufoTexture);

	runway_sprite[0].setPosition(returnX("RWY-A"), 400);
	runway_sprite[1].setPosition(returnX("RWY-B"), 400);
	runway_sprite[2].setPosition(returnX("RWY-C"), 400);

	Text inputText;
    inputText.setFont(font);
	inputText.setCharacterSize(24);
	inputText.setFillColor(Color::Black);
 	inputText.setPosition(650, 150);
	string userInput = ""; 

	///////////////////////////////////////////////////////////////////////////////



    const float radius = 50.0f;
    const Vector2f center(1100.0f, 120.0f);
    CircleShape radarBase;
    vector<CircleShape> radarCircles;
    RectangleShape sweep;
    float sweepAngle = 0.0f;
    const float sweepSpeed = 90.0f; // degrees per second
    Clock clock1;

    // Initialize radar
    setupRadar(radarBase, radarCircles, sweep, radius, center);

	Text time_text;
    time_text.setFont(font);
    time_text.setCharacterSize(24);
    time_text.setFillColor(Color::White);
	time_text.setString(to_string(RealTime));
    time_text.setPosition(1070, 30);


	///////////////////////////////////////////////////////////////////////////////


	initialize_YText();

	pipe(fd1);
	pipe(fd2);
	pipe(fd3);
	pipe(fd4);
	pipe(fd5);
	pipe(fd6);

	sem_init(&sem, 0, 1);
	sem_init(&sem2, 0, 0);

	pid_t mainP = fork();

	if (mainP > 0)  // -----ATCS PROCESS-----
	{
		close(fd1[0]);
		close(fd2[0]); close(fd2[1]);
		close(fd3[0]); close(fd3[1]);
		close(fd4[0]); close(fd4[1]);
		close(fd5[0]); close(fd5[1]);
		close(fd6[1]);
		// Simulation Loop
		while (window.isOpen())
		{

			sf::Event event;
			while (window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed)
					window.close();

				if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space)
				{
					window.close();
				}


				if (event.type == sf::Event::TextEntered) {
					if (event.text.unicode < 128) { 
						if (event.text.unicode == 8 && !userInput.empty()) { 
							userInput.pop_back();
						} else if (event.text.unicode >= 32 && event.text.unicode <= 126) { 
							userInput += static_cast<char>(event.text.unicode);
						} else if (event.text.unicode == 13) { 
							std::cout << "User entered: " << userInput << std::endl;
							userInput.clear(); 
						}
						inputText.setString(userInput);
					}
	            }
			
			}

			float deltaTime = clock.restart().asSeconds();

			// Update sweep angle
			sweepAngle += sweepSpeed * deltaTime;
			if (sweepAngle > 360.0f) sweepAngle -= 360.0f;
			sweep.setRotation(sweepAngle);

			time_text.setString(to_string(int(RealTime / 60)) + ":" + to_string(int(RealTime) % 60));


			window.clear(Color::White);

			window.draw(background);

			window.draw(radarBase);
			for (auto& ring : radarCircles)
				window.draw(ring);
			window.draw(sweep);

			for (int i = 0; i < 3; ++i)
			{
				if (RWY[i].f != NULL)
					SFML_Stuff(RWY[i]);

				window.draw(runway_sprite[i]);
				window.draw(planes_sprite[i]);
				window.draw(text[i]);
			}
			window.draw(inputText);
			window.draw(time_text);
			window.draw(Atc_Sprite);

			window.display();

			//////////////////////////////////////////////////////////////////////

			if (!arrival.empty() && arrival.front()->time_minute <= ::min && arrival.front()->time_second <= ::sec)
			{
				while (!arrival.empty() && arrival.front()->time_minute <= ::min && arrival.front()->time_second <= ::sec)
				{
					Flight *f = arrival.front();
					arrival.pop();
					pthread_create(&arrival_flights[a_count], NULL, RunwayWaitQueue, (void *)f);
					pthread_detach(arrival_flights[a_count]);
					a_count++;
				}
			}
			if (!departure.empty() && departures > 0 && departure.front()->time_minute <= ::min && departure.front()->time_second <= ::sec)
			{
				while (!departure.empty() && departure.front()->time_minute <= ::min && departure.front()->time_second <= ::sec)
				{
					Flight *f = departure.front();
					departure.pop();
					pthread_create(&departure_flights[d_count], NULL, RunwayWaitQueue, (void *)f);
					pthread_detach(departure_flights[d_count]);
					d_count++;
				}
			}
			if (RealTime <= 0)
				break;
		}
	}
	else if (mainP == 0)
	{
		pid_t mainC = fork();
		if (mainC > 0)  //-----AVN PROCESS-----
		{
			close(fd1[1]);
			close(fd2[0]);
			close(fd3[0]); close(fd3[1]);
			close(fd4[0]); close(fd4[1]);
			close(fd5[1]); 
			close(fd6[0]);
			do
			{
				char message[5000];
				read(fd1[0], message, 5000);
				string res(message);
				string field;
            int fieldIndex = 0;
            int avn_id,speed_recorded;
            string payment_status,aircraft_type,airline, flight_number;
    for (size_t i = 0; i < res.length(); ++i) {
        if (res[i] == '|' || i == res.length() - 1) {
            if (i == res.length() - 1 && res[i] != '|') {
                field += res[i]; // Include last character if not a delimiter
            }
            switch (fieldIndex) {
                case 0: avn_id = stoi(field); break;
                case 1: flight_number = field; break;
                case 2: airline = field; break;
                case 3: aircraft_type = field; break;
                case 4: speed_recorded = stoi(field); break;
                case 5: payment_status = field; break;
            }
            field.clear();
            fieldIndex++;
            continue;
        }
        field.push_back(res[i]);
    }
                float fine = 0, tax = 0, total=0;
                                if(aircraft_type == "Commercial"){
                                  fine = 500000;
                                  tax = 75000;
                                }else{
                                  fine = 700000;
                                  tax = 105000;
                                }
                                total=fine+tax;
                                cout << "---------------------------------------------" << endl;
                                cout << "----------------( AVN REPORT )---------------"<<endl<<endl;
				cout << "AVN ID : " << avn_id << endl;
				cout << "Flight Number : " << flight_number << endl;
				cout << "Airline Name : " << airline << endl;
				cout << "Aircraft : " << aircraft_type << endl;
				cout << "Speed Recorded : " <<speed_recorded << endl; 
				cout << "Fine : " << fine << endl;
				cout << "Tax (15%) : " << tax << endl;
				cout << "Total Amount : " << total << endl;
				cout << "Payment Status : " << payment_status << endl;
				cout << "---------------------------------------------" << endl << endl;
				string ta=to_string(total);
				write(fd2[1], ta.c_str(), strlen(ta.c_str())+1);
				read(fd5[0], message, sizeof(message));
				string reply(message);
				if(reply=="Y"){
				  //File LOG Handling with payment status Paid
				  fstream f;
				  f.open("Log.txt",ios::out|ios::app);
				  f<< "AVN ID : " << avn_id << endl;
				f << "Flight Number : " << flight_number << endl;
				f << "Airline Name : " << airline << endl;
				f << "Aircraft : " << aircraft_type << endl;
				f << "Speed Recorded : " <<speed_recorded << endl; 
				f << "Fine : " << fine << endl;
				f << "Tax (15%) : " << tax << endl;
				f << "Total Amount : " << total << endl;
				f << "Payment Status : " << "Paid" << endl;
				f<<"---------------------------------------"<<endl;
				  write(fd6[1], "Y", 2);
				}
				else{
				//File LOG Handling with payment status UnPaid
				  fstream f;
				  f.open("Log.txt",ios::out|ios::app);
				  f<< "AVN ID : " << avn_id << endl;
				f << "Flight Number : " << flight_number << endl;
				f << "Airline Name : " << airline << endl;
				f << "Aircraft : " << aircraft_type << endl;
				f << "Speed Recorded : " <<speed_recorded << endl; 
				f << "Fine : " << fine << endl;
				f << "Tax (15%) : " << tax << endl;
				f << "Total Amount : " << total << endl;
				f << "Payment Status : " << "Unpaid" << endl;
				f<<"---------------------------------------"<<endl;
				  write(fd6[1], "n", 2);
				}
				
			} while (1);
		}
		else if (mainC == 0)
		{
			pid_t mainC2 = fork();
			
			if (mainC2 > 0)  //-----STRIPE PAY PROCESS-----
			{
			
				close(fd1[0]); close(fd1[1]);
				close(fd2[1]);
				close(fd3[0]);
				close(fd4[1]); 
				close(fd5[0]); 
				close(fd6[0]); close(fd6[1]);
				do{
					char message[5000];
					read(fd2[0], message, sizeof(message));
					string ta(message);
					write(fd3[1], ta.c_str(), strlen(ta.c_str())+1);
					read(fd4[0], message, sizeof(message));
					string reply(message);
					if(reply == "Y") {
					  cout << "Fine Cleared, Amount has been submitted" << endl;
					  write(fd5[1], "Y", 2);
					}
					else {
					  cout << "Fine Not Cleared, Adding to records" << endl;
					  write(fd5[1], "n", 2);
					}
				}while(1);
			}
			else if (mainC2 == 0)  //-----AIRLINE PORTAL PROCESS-----
			{
				close(fd1[0]); close(fd1[1]);
				close(fd2[1]); close(fd2[0]);
				close(fd3[1]);
				close(fd4[0]); 
				close(fd5[0]); close(fd5[1]); 
				close(fd6[0]); close(fd6[1]);
				do{
					char message[5000];
					string id;
					string ch;
					read(fd3[0], message, sizeof(message));
					cout << "---------------------------------------------" << endl;
                                        cout << "-------------( AIRLINE PORTAL )--------------"<<endl<<endl;
					cout << "Enter Flight ID : ";
					cin >> id;
					fstream f;
					f.open("Log.txt",ios::in);
					while(!f.eof()){
					  char ch;
					  f.get(ch);
					  cout<<ch;
					}
					cout << "Do You Want To Clear Current Fine [Y/n] : ";
					cin>>ch;
					write(fd4[1], ch.c_str(), strlen(ch.c_str())+1);
				}while(1);
			}
			
		}
	}

	return 0;
}
