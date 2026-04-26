#include "enviroment.h"
using namespace std;
//CarState::CarState(map<string, string> conf) {
    //len = stof(conf["LEN_CAR"]);
    //width = stof(conf["WIDTH_CAR"]);
//}


QPolygonF Enviroment::car_polygon() {
    float sin_t = sin(car.theta);
    float cos_t = cos(car.theta);

    float xA = car.x + 0.5*len_car*cos_t + 0.5*width_car*sin_t;
    float yA = car.y + 0.5*len_car*sin_t - 0.5*width_car*cos_t;

    float xB = car.x + 0.5*len_car*cos_t - 0.5*width_car*sin_t;
    float yB = car.y + 0.5*len_car*sin_t + 0.5*width_car*cos_t;

    float xC = car.x - 0.5*len_car*cos_t - 0.5*width_car*sin_t;
    float yC = car.y - 0.5*len_car*sin_t + 0.5*width_car*cos_t;

    float xD = car.x - 0.5*len_car*cos_t + 0.5*width_car*sin_t;
    float yD = car.y - 0.5*len_car*sin_t - 0.5*width_car*cos_t;


    QVector<QPointF> vertices = {
        QPointF(xA, yA),  // Top-right corner
        QPointF(xB, yB),  // bottom-right corner
        QPointF(xC, yC),  // bottom-left corner
        QPointF(xD, yD),  // Top-left corner
    };

    QPolygonF rectangle(vertices);

    return rectangle;
}



CarState::~CarState() {}



CarState CarState::compute_new_state(float speed, float steering, int timestep, float len, float front_overhang, float rear_overhang, bool approx_motion) {
    float dt = timestep / 1000.0;

    if(approx_motion) {
        float x_r = x - 0.5 * len * cos(theta); // x ruota dietro
        float y_r = y - 0.5 * len * sin(theta); // y ruota dietro

        float x_r_next = x_r + dt * cos(theta) * speed;
        float y_r_next = y_r + dt * sin(theta) * speed;

        float x_next = x_r_next + 0.5*len*cos(theta);
        float y_next = y_r_next + 0.5*len*sin(theta);
        float theta_next = theta - dt * sin(steering) * speed / len;

        return CarState(x_next, y_next, theta_next);
    }
    else {
        if(steering==0.0) { // if there is no steering, rotation do not make sense
            return CarState(
                x + dt*speed*cos(theta), 
                y + dt*speed*sin(theta),
                theta
            );
        }
        // angle of rotation
        float alpha = dt*speed*tan(steering)/(len-front_overhang-rear_overhang);
        // center of rotation
        float x_cr = x - (0.5*len-rear_overhang)*cos(theta) - (len-front_overhang-rear_overhang)*sin(theta)/tan(steering);
        float y_cr = y - (0.5*len-rear_overhang)*sin(theta) + (len-front_overhang-rear_overhang)*cos(theta)/tan(steering);
        // next state as rotation of (x, y, theta) respect to (x_cr, y_cr) of angle alpha
        float x_next = (x-x_cr)*cos(alpha) - (y-y_cr)*sin(alpha) + x_cr;
        float y_next = (x-x_cr)*sin(alpha) + (y-y_cr)*cos(alpha) + y_cr;
        float theta_next = theta+alpha;
        
        if(isnan(x_next)) {
            cout << "TROVATO NAN" << " alpha: " << alpha << " x_cr: " <<x_cr<< " y_cr: " <<y_cr << endl;
        }

        return CarState(x_next, y_next, theta_next);
    }
}





Enviroment::Enviroment() {
    car = CarState();
    //float len_slot = len_car + 2 * tol;
    //float width_slot = width_car + tol;
    QVector<QPointF> vertices = {
        QPointF(0, 0),  // top-left corner
        QPointF(0, len_env),  //   Bottom-left corner
        QPointF(width_env-width_slot, len_env),  // Bottom-right corner
        QPointF(width_env-width_slot, len_env-free_park*len_slot), // PARK CORNER  bottom-right
        QPointF(width_env, len_env-free_park*len_slot), // PARK CORNER  bottom-left
        QPointF(width_env, len_env-(free_park+1)*len_slot), // PARK CORNER  top-left
        QPointF(width_env-width_slot, len_env-(free_park+1)*len_slot),  // PARK CORNER  top-right
        QPointF(width_env-width_slot, 0),  // Top-right corner
    };
    env_polygon = QPolygonF(vertices);

}

Enviroment::Enviroment(map<string, string> conf) {
    car = CarState();

    len_car = stof(conf["LEN_CAR"]);
    width_car = stof(conf["WIDTH_CAR"]);

    len_env = stof(conf["LEN_ENV"]);
    width_env = stof(conf["WIDTH_ENV"]);

    len_slot = stof(conf["LEN_SLOT"]);
    width_slot = stof(conf["WIDTH_SLOT"]);

    front_overhang = stof(conf["FRONT_OVERHANG"]);
    rear_overhang = stof(conf["REAR_OVERHANG"]);

    free_park = stof(conf["FREE_PARK"]);

    reward_for_hit = stof(conf["REWARD_FOR_HIT"]);
    reward_for_park = stof(conf["REWARD_FOR_PARK"]);
    reward_for_nothing = stof(conf["REWARD_FOR_NOTHING"]);

    x_divide = stoi(conf["X_DIVIDE"]);
    y_divide = stoi(conf["Y_DIVIDE"]);
    theta_divide = stoi(conf["THETA_DIVIDE"]);


    approx_motion = (conf["APPROX_MOTION"] == "1");


    //float len_slot = len_car + 2 * tol;
    //float width_slot = width_car + tol;
    QVector<QPointF> vertices = {
        QPointF(0, 0),  // top-left corner
        QPointF(0, len_env),  //   Bottom-left corner
        QPointF(width_env-width_slot, len_env),  // Bottom-right corner
        QPointF(width_env-width_slot, len_env-free_park*len_slot), // PARK CORNER  bottom-right
        QPointF(width_env, len_env-free_park*len_slot), // PARK CORNER  bottom-left
        QPointF(width_env, len_env-(free_park+1)*len_slot), // PARK CORNER  top-left
        QPointF(width_env-width_slot, len_env-(free_park+1)*len_slot),  // PARK CORNER  top-right
        QPointF(width_env-width_slot, 0),  // Top-right corner
    };
    env_polygon = QPolygonF(vertices);

}

int Enviroment::discretize_state() {
    
    float x_section = (float)(width_env) / x_divide;
    float y_section = (float)(len_env) / y_divide;
    float theta_section = 2*M_PI / theta_divide;

    int x_state = (int)(car.x / x_section);
    if(x_state >= x_divide) x_state = x_divide - 1;
    int y_state = (int)(car.y / y_section);
    if(y_state >= y_divide) y_state = y_divide - 1;
    float theta_mod = fmod(car.theta, 2.0 * M_PI);
    if (theta_mod < 0)
        theta_mod += 2.0 * M_PI;
    int theta_state = (int)(theta_mod / theta_section);
    if(theta_state >= theta_divide) theta_state = theta_divide -1;
    int state_ret = theta_state + theta_divide * y_state + theta_divide * y_divide * x_state;




    if(state_ret >= x_divide*y_divide*theta_divide) {
        cout << "ERRORE, stato:" << endl;
        cout << x_state << ", " << y_divide << ", " << theta_divide << endl;
        return -1;
    }
    return state_ret;

}


bool Enviroment::car_allowed() {
    QPolygonF car_rect = car_polygon();

    for(auto point: car_rect) {
        if(!env_polygon.containsPoint(point, Qt::OddEvenFill)) {
            return false;
        }
    }
    if(car_rect.containsPoint(env_polygon[3], Qt::OddEvenFill)) {
        return false;
    }
    if(car_rect.containsPoint(env_polygon[6], Qt::OddEvenFill)) {
        return false;
    }

    return true;
}


bool Enviroment::car_parked() {
    QVector<QPointF> car_rect = car_polygon();

    QPolygonF parkspace({env_polygon[3], env_polygon[4], env_polygon[5], env_polygon[6]});

    for(auto point: car_rect) {
        if(!parkspace.containsPoint(point, Qt::OddEvenFill)) return false;
    }
    if(anglediff(-M_PI*0.5, car.theta) > M_PI/6.0) return false;
    return true;
}


float Enviroment::reward() {
    float r;
    if(!car_allowed()) r= reward_for_hit;
    else if(car_parked()) r= reward_for_park;
    else r= reward_for_nothing;
    return r;
}

Enviroment Enviroment::compute_new_state(float speed, float steering, int timestep) {
    Enviroment e = *this;
    e.car = e.car.compute_new_state(
        speed, 
        steering, 
        timestep, 
        len_car, 
        front_overhang,
        rear_overhang,
        approx_motion
    );
    return e;
}

void Enviroment::set_random_carstate() {
    float x_new = randomFloat(width_car*0.6, width_env - width_car*0.6 - width_slot);
    float y_new = randomFloat(len_car*0.6, len_env - len_car*0.6);
    float theta_new = -M_PI*0.5;
    //if(randomFloat(0.0, 1.0)>0.5) theta_new = -M_PI*0.5;

    car.x = x_new;
    car.y = y_new;
    car.theta = theta_new;
}