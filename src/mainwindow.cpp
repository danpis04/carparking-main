#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <chrono>
#include <QMessageBox>
#include <QStatusBar>
#include <QMenuBar>
#include <QApplication>
#include <QGroupBox>
#include <QResizeEvent>
#include <algorithm>
// log constant
#define LOG_FREQ 1000000 // how many iterations between each log
#define WHEEL_RADIUS 0.3
#define SHOW_WHEEL 1
using namespace std;

QPoint map_into_window(const QPointF &p, int m, int r) {
    return QPoint((int)(m + r*p.x()), (int)(m + r*p.y()));
}

QPolygon map_into_window(const QPolygonF &p, int m, int r) {
    QPolygon polygon;

    for(auto point: p) {
        polygon << map_into_window(point, m, r);
    }

    return polygon;
}

QLine map_into_window(const QLineF &l, int m, int r) {
    return QLine(map_into_window(l.p1(), m, r), map_into_window(l.p2(), m, r));
}

QVector<QLine> map_into_window(const QVector<QLineF> &v, int m, int r) {
    QVector<QLine> vi;
    for(auto line: v) {
        vi.push_back(map_into_window(line, m, r));
    }
    return vi;
}

MainWindow::MainWindow(QWidget *parent): 
    QMainWindow(parent), 
    ui(new Ui::MainWindow), 
    iter(0), 
    hit_counter(0), 
    success_counter(0),
    last_speed_action(0),
    last_steering_action(0),
    avg_tdr(0)
{
    setlocale(LC_NUMERIC, "C");
    conf = readConfig("configuration/default.conf");

    int n_steering_actions = stoi(conf["N_STEERING_ACTIONS"]);
    int n_speed_actions = stoi(conf["N_SPEED_ACTIONS"]);
    conf["N_ACTIONS"] = to_string(n_steering_actions*n_speed_actions);
    conf["N_STATES"] = to_string(stoi(conf["X_DIVIDE"]) * stoi(conf["Y_DIVIDE"]) * stoi(conf["THETA_DIVIDE"]));
    cout << stoi(conf["N_STATES"])*stoi(conf["N_ACTIONS"])  << endl;
    controller = QLearningModel(conf);

    ui->setupUi(this);
    setWindowTitle("Animation Controller");

    resize(stoi(conf["WINDOW_WIDTH"]), stoi(conf["WINDOW_HEIGHT"]));

    msec = stoi(conf["MSEC"]);
    animation_speed = stoi(conf["ANIMATION_SPEED"]);
    time_ratio = stoi(conf["TIME_RATIO"]);

    speed_actions = progression(stoi(conf["N_SPEED_ACTIONS"]), stof(conf["SPEED_UNITY"]));
    steering_actions = progression(stoi(conf["N_STEERING_ACTIONS"]), M_PI*stof(conf["STEERING_UNITY"])/180);



    ui->learning_rate->setValue(controller.lr_max);
    ui->discount_factor->setValue(controller.discount_factor);
    ui->epsilon->setValue(controller.er_max);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        if(iter % time_ratio == 0) {
            model_iteration();
        }
        enviroment_iteration(animation_speed*msec);
        iter++;
        update();
    });

    timer->setInterval(msec); 

    env = Enviroment(conf);
    env.set_random_carstate();
    state_encoded = env.discretize_state();


    int m = stoi(conf["MARGIN"]);
    int r = stoi(conf["PIXEL_RATIO"]);
    car_picture = map_into_window(env.car_polygon(), m, r);
    env_picture = map_into_window(env.env_polygon, m, r);

    // Anchor right-panel group boxes to the right edge for the initial layout —
    // resize() above may not fire resizeEvent if size matches the .ui geometry.
    layoutControls();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_playButton_clicked()
{
    controller.lr = ui->learning_rate->value();
    controller.er = ui->epsilon->value();
    controller.er_max = ui->epsilon->value();

    timer->start();

    ui->statusLabel->setText("Animation running. Use Pause to suspend, Stop to reset the car.");
    statusBar()->showMessage("Animation running");
}

void MainWindow::on_pauseButton_clicked()
{
    timer->stop();
    ui->statusLabel->setText("Animation paused.");
    statusBar()->showMessage("Animation paused");
}

void MainWindow::on_stopButton_clicked()
{
    env.set_random_carstate();
    int m = stoi(conf["MARGIN"]);
    int r = stoi(conf["PIXEL_RATIO"]);
    car_picture = map_into_window(env.car_polygon(), m, r);
    update();
    timer->stop();

    ui->statusLabel->setText("Animation stopped. Car re-initialized to a random pose.");
    statusBar()->showMessage("Animation stopped");
}


void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    layoutControls();
}

void MainWindow::layoutControls()
{
    if (!ui || !ui->qtableGroup) return;

    int W = centralWidget()->width();
    int H = centralWidget()->height();

    const int rightW = 440;
    const int rightMargin = 10;
    int panelX = std::max(200, W - rightW - rightMargin);

    QList<QGroupBox*> groups = {
        ui->hyperGroup, ui->trainGroup, ui->animGroup, ui->qtableGroup
    };
    for (auto *g : groups) {
        QRect r = g->geometry();
        g->setGeometry(panelX, r.y(), rightW, r.height());
    }

    int statusH = 50;
    int statusW = std::max(150, panelX - 20);
    int statusY = std::max(0, H - statusH - 10);
    ui->statusLabel->setGeometry(10, statusY, statusW, statusH);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // ----- Compute the available drawing rectangle in window coords -----
    int top = menuBar() ? menuBar()->height() : 0;
    int bottomReserved = (statusBar() ? statusBar()->height() : 0) + 60; // status label + gap
    int rightPanelLeft = std::max(200, width() - 440 - 10);

    int avail_x = 0;
    int avail_y = top + 5;
    int avail_w = std::max(50, rightPanelLeft - 10 - avail_x);
    int avail_h = std::max(50, height() - bottomReserved - avail_y);

    // ----- Fit env (width_env x len_env) into available area, preserving aspect -----
    const int min_pad = 8;
    int r_x = (avail_w - 2*min_pad) / std::max(1, (int)env.width_env);
    int r_y = (avail_h - 2*min_pad) / std::max(1, (int)env.len_env);
    int r = std::min(r_x, r_y);
    if (r < 5) r = 5;

    int draw_w = (int)(r * env.width_env);
    int draw_h = (int)(r * env.len_env);
    int m_x = avail_x + (avail_w - draw_w) / 2;
    int m_y = avail_y + (avail_h - draw_h) / 2;

    // map_into_window already adds an offset; pass m=0 and translate the painter
    const int m = 0;
    painter.save();
    painter.translate(m_x, m_y);

    // ----- Lot polygon (black fill, blue boundary) -----
    {
        QPen lotPen(Qt::blue);
        lotPen.setWidth(3);
        painter.setPen(lotPen);
    }
    painter.setBrush(Qt::black);
    painter.drawPolygon(map_into_window(env.env_polygon, m, r));

    // ----- Car body (red) -----
    painter.setPen(Qt::black);
    painter.setBrush(Qt::red);
    car_picture = map_into_window(env.car_polygon(), m, r);
    painter.drawPolygon(car_picture);

    // ----- Centre and rear-axle dots (yellow, contrasts against red car) -----
    painter.setBrush(Qt::yellow);
    painter.setPen(Qt::yellow);
    painter.drawEllipse(map_into_window(QPointF(env.car.x, env.car.y), m, r), 2, 2);

    float sin_t = sin(env.car.theta);
    float cos_t = cos(env.car.theta);
    float xR = env.car.x - (0.5*env.len_car-env.rear_overhang)*cos_t;
    float yR = env.car.y - (0.5*env.len_car-env.rear_overhang)*sin_t;
    painter.drawEllipse(map_into_window(QPointF(xR, yR), m, r), 2, 2);

    if (SHOW_WHEEL) {
        QPen pen(Qt::green);
        pen.setWidth(3);
        painter.setPen(pen);

        float xRR = xR - 0.5*env.width_car*sin_t;
        float yRR = yR + 0.5*env.width_car*cos_t;
        QLineF RR(
            xRR - WHEEL_RADIUS*cos_t,
            yRR - WHEEL_RADIUS*sin_t,
            xRR + WHEEL_RADIUS*cos_t,
            yRR + WHEEL_RADIUS*sin_t
        );
        painter.drawLine(map_into_window(RR, m, r));

        float xRL = xR + 0.5*env.width_car*sin_t;
        float yRL = yR - 0.5*env.width_car*cos_t;
        QLineF RL(
            xRL - WHEEL_RADIUS*cos_t,
            yRL - WHEEL_RADIUS*sin_t,
            xRL + WHEEL_RADIUS*cos_t,
            yRL + WHEEL_RADIUS*sin_t
        );
        painter.drawLine(map_into_window(RL, m, r));

        float sin_t_a = sin(env.car.theta+steering_actions[last_steering_action]);
        float cos_t_a = cos(env.car.theta+steering_actions[last_steering_action]);
        float xFR = xRR + (env.len_car-env.front_overhang-env.rear_overhang)*cos_t;
        float yFR = yRR + (env.len_car-env.front_overhang-env.rear_overhang)*sin_t;
        QLineF FR(
            xFR - WHEEL_RADIUS*cos_t_a,
            yFR - WHEEL_RADIUS*sin_t_a,
            xFR + WHEEL_RADIUS*cos_t_a,
            yFR + WHEEL_RADIUS*sin_t_a
        );
        painter.drawLine(map_into_window(FR, m, r));

        float xFL = xRL + (env.len_car-env.front_overhang-env.rear_overhang)*cos_t;
        float yFL = yRL + (env.len_car-env.front_overhang-env.rear_overhang)*sin_t;
        QLineF FL(
            xFL - WHEEL_RADIUS*cos_t_a,
            yFL - WHEEL_RADIUS*sin_t_a,
            xFL + WHEEL_RADIUS*cos_t_a,
            yFL + WHEEL_RADIUS*sin_t_a
        );
        painter.drawLine(map_into_window(FL, m, r));

        // Wheel-axle dots — yellow for visibility on the red body
        painter.setBrush(Qt::yellow);
        painter.setPen(Qt::yellow);
        painter.drawEllipse(map_into_window(QPointF(xRR, yRR), m, r), 2, 2);
        painter.drawEllipse(map_into_window(QPointF(xRL, yRL), m, r), 2, 2);
        painter.drawEllipse(map_into_window(QPointF(xFR, yFR), m, r), 2, 2);
        painter.drawEllipse(map_into_window(QPointF(xFL, yFL), m, r), 2, 2);
    }

    painter.restore();
}



void MainWindow::enviroment_iteration(int timestep) {
    //cout << "--ENV ITERATION--" << endl;
    int speed_action = last_speed_action;
    int steering_action = last_steering_action;

    Enviroment new_env_st = env.compute_new_state(speed_actions[speed_action], steering_actions[steering_action], timestep);

    int new_state_encoded = new_env_st.discretize_state();


    if(!new_env_st.car_allowed()) {
        hit_counter++;
        env.set_random_carstate();
        state_encoded = env.discretize_state(); 
        int action = controller.chooseAction(state_encoded, ui->eval->isChecked());
        last_speed_action = action / steering_actions.size();
        last_steering_action = action % steering_actions.size();
    }
    else if(new_env_st.car_parked()) {
        success_counter++;
        //env.car = CarState::generate_random_state();
        env.set_random_carstate();
        state_encoded = env.discretize_state();
        int action = controller.chooseAction(state_encoded, ui->eval->isChecked());
        last_speed_action = action / steering_actions.size();
        last_steering_action = action % steering_actions.size();
    }

    else {
        env = new_env_st;
        state_encoded = new_state_encoded;
    }
}

void MainWindow::model_iteration() {
    int action = controller.chooseAction(state_encoded, ui->eval->isChecked());
    last_speed_action = action / steering_actions.size();
    last_steering_action = action % steering_actions.size();

    Enviroment new_env_st = env.compute_new_state(speed_actions[last_speed_action], steering_actions[last_steering_action], msec*time_ratio * animation_speed);

    int new_state_encoded = new_env_st.discretize_state();//auto new_state_encoded = som.findBMU(new_car_st_vect);

    if(!ui->eval->isChecked()) {

        float reward = new_env_st.reward();
        
        float tdr = controller.train(state_encoded, action, reward, new_state_encoded);

        //cout << "Reward, tdr: " << reward << ", " << tdr << endl;
        int i = iter/time_ratio;
        //cout << i << endl;
        controller.er = controller.er_min + controller.er_ratio*(controller.er - controller.er_min);
        controller.lr = controller.lr_min + controller.lr_ratio*(controller.lr - controller.lr_min);
        if(i % 1000 == 0) {
            //cout << i << ", " << controller.er << endl;
        }
        avg_tdr = (i*avg_tdr+abs(tdr))/(i+1);
    }


}






void MainWindow::on_trainButton_clicked()
{
    controller.lr = ui->learning_rate->value();
    controller.er = ui->epsilon->value();
    controller.er_max = ui->epsilon->value();

    bool eval_mode = ui->eval->isChecked();
    QString mode = eval_mode ? "Testing" : "Training";

    ui->trainButton->setEnabled(false);
    ui->trainButton->setText(mode + " in progress...");
    ui->statusLabel->setText(mode + " in progress — UI will be unresponsive until it completes.");
    statusBar()->showMessage(mode + " in progress...");
    QApplication::processEvents();

    ofstream file("log/log12.txt");
    if (!file.is_open()) {
        cerr << "Failed to open file: log/log12.txt" << endl;
    }

    int num_iter = ui->num_iterations->value();
    int total_hits = 0;
    int total_successes = 0;
    for(int i=0; i<num_iter; ++i) {
        model_iteration();
        enviroment_iteration(animation_speed*msec*time_ratio);
        iter += time_ratio;
        if((i+1)%LOG_FREQ==0) {
            cout << "{\"iteration\": \"" << i+1 << "\", \"hit\": \"" << hit_counter << "\", \"success\": \"" << success_counter << "\", \"success_ratio\": \""<< 100 * success_counter / (float)(success_counter+hit_counter) << '%' << "\", \"lr\": \""<< controller.lr << "\", \"er\": \"" << controller.er << "\", \"avg_tdr\": \"" << avg_tdr <<"\"}"<< endl;
            file << "{\"iteration\": \"" << i+1 << "\", \"hit\": \"" << hit_counter << "\", \"success\": \"" << success_counter << "\", \"success_ratio\": \""<< 100 * success_counter / (float)(success_counter+hit_counter) << '%' << "\", \"lr\": \""<< controller.lr << "\", \"er\": \"" << controller.er << "\", \"avg_tdr\": \"" << avg_tdr <<"\"}"<< endl;
            total_hits += hit_counter;
            total_successes += success_counter;
            hit_counter=0;
            success_counter=0;
            avg_tdr = 0;
            update();
        }
    }
    total_hits += hit_counter;
    total_successes += success_counter;

    cout << "Trained "<< num_iter <<endl;
    file.close();

    ui->trainButton->setEnabled(true);
    ui->trainButton->setText("Run Train / Test");

    int total_episodes = total_hits + total_successes;
    float success_pct = total_episodes > 0
        ? 100.0f * total_successes / total_episodes
        : 0.0f;

    QString summary = QString("%1 done.\n\n"
                              "Iterations: %2\n"
                              "Successful parks: %3\n"
                              "Collisions: %4\n"
                              "Success ratio: %5%\n\n"
                              "Per-million-iteration stats logged to:\n"
                              "  log/log12.txt   (JSON lines)\n"
                              "  stdout.log      (mirror of stdout)\n"
                              "Errors (if any) in stderr.log.")
        .arg(mode)
        .arg(num_iter)
        .arg(total_successes)
        .arg(total_hits)
        .arg(QString::number(success_pct, 'f', 2));

    ui->statusLabel->setText(mode + " done — see log/log12.txt for details.");
    statusBar()->showMessage(mode + " done", 5000);

    QMessageBox::information(this, mode + " complete", summary);
}


void MainWindow::on_resetButton_clicked() {
    controller.reset();
    cout << "Q-Table reset" << endl;
    ui->statusLabel->setText("Q-Table reset to random values.");
    statusBar()->showMessage("Q-Table reset", 4000);
}

void MainWindow::on_qtable_load_sp_clicked()
{
    std::string fname = ui->file_name->text().toStdString();
    if (fname.empty()) {
        ui->statusLabel->setText("Load failed: file name is empty.");
        statusBar()->showMessage("Load failed: empty file name", 4000);
        return;
    }
    bool ok = controller.loadWeights(fname);
    if (ok) {
        ui->statusLabel->setText(QString("Q-Table loaded from parameters/%1").arg(QString::fromStdString(fname)));
        statusBar()->showMessage("Q-Table loaded", 4000);
    } else {
        ui->statusLabel->setText(QString("Failed to load parameters/%1 (see stderr.log).").arg(QString::fromStdString(fname)));
        statusBar()->showMessage("Load failed", 4000);
    }
}

void MainWindow::on_qtable_store_sp_clicked()
{
    std::string fname = ui->file_name->text().toStdString();
    if (fname.empty()) {
        ui->statusLabel->setText("Store failed: file name is empty.");
        statusBar()->showMessage("Store failed: empty file name", 4000);
        return;
    }
    bool ok = controller.storeWeights(fname);
    if (ok) {
        ui->statusLabel->setText(QString("Q-Table stored to parameters/%1").arg(QString::fromStdString(fname)));
        statusBar()->showMessage("Q-Table stored", 4000);
    } else {
        ui->statusLabel->setText(QString("Failed to store parameters/%1 (does the directory exist? see stderr.log).").arg(QString::fromStdString(fname)));
        statusBar()->showMessage("Store failed", 4000);
    }
}
