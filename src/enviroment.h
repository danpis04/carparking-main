#ifndef ENVIROMENT_H
#define ENVIROMENT_H

#include <QPolygon>
#include <QLineF>
#include "utils.h"
#include <vector>
#include <map>
#include <string>

/**
 * @brief Class that contains the state of the car.
 * 
 * The CarState class represents the state of a car in terms of its position (x, y),
 * orientation (theta), and provides methods to calculate new states, check
 * validity, and compute rewards based on the car's configuration.
 */
class CarState {
public:
    float x;     ///< The x-coordinate of the car.
    float y;     ///< The y-coordinate of the car.
    float theta; ///< The orientation of the car in radians (angle).
    //float len;
    //float width;
    /**
     * @brief Default constructor for CarState.
     * 
     * Initializes the car's state with default values (x = 0, y = 0, theta = 0).
     */
    CarState() : x(0), y(0), theta(0) {};


    /**
     * @brief Default constructor for CarState.
     * 
     * Initializes the car's state with default values (x = 0, y = 0, theta = 0).
     */
    //CarState(std::map<std::string, std::string> conf);



    /**
     * @brief Constructor for CarState with specified values.
     * 
     * @param x The x-coordinate of the car.
     * @param y The y-coordinate of the car.
     * @param theta The orientation (angle) of the car in radians.
     */
    CarState(float x, float y, float theta) : x(x), y(y), theta(theta) {};

    /**
     * @brief Converts the car's state to a polygon representation.
     * 
     * @return A QPolygonF representing the car's shape in the current state.
     */
    //QPolygonF to_polygon();


    /**
     * @brief Computes the car's new state based on speed and steering input.
     * 
     * Calculates the new position and orientation of the car given the speed, 
     * steering angle, and time step.
     * 
     * @param speed The speed of the car.
     * @param steering The steering angle in radians.
     * @param timestep The time step for the state change.
     * @return The new CarState after applying the inputs.
     */
    CarState compute_new_state(float speed, float steering, int timestep, float len, float front_overhang, float rear_overhang, bool approx_motion);


    /**
     * @brief Destructor for CarState.
     * 
     * Cleans up resources associated with the car state.
     */
    ~CarState();
};




class Enviroment {
public:
    CarState car;
    QPolygonF env_polygon;

    float len_env;   // y-axis  
    float width_env; // x-axis
    
    float len_slot;
    float width_slot;

    float front_overhang;
    float rear_overhang;

    float free_park;

    float len_car;
    float width_car;

    float reward_for_hit;
    float reward_for_park;
    float reward_for_nothing;
    int x_divide;
    int y_divide;
    int theta_divide;
    bool approx_motion;


    Enviroment();
    Enviroment(std::map<std::string, std::string> conf);
    bool car_parked();
    bool car_allowed();
    float reward();
    void set_random_carstate();
    int discretize_state();
    QPolygonF car_polygon();
    Enviroment compute_new_state(float speed, float steering, int timestep);
};



#endif // ENVIROMENT_H
