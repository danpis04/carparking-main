#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QRect>
#include <QPainter>
#include <QPoint>
#include <QPolygon>
#include <iostream>
#include <cmath>
#include "enviroment.h"
#include "q_learning.h"
#include <QFileDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief The MainWindow class represents the main interface for the application.
 * 
 * This class inherits from QMainWindow and is responsible for handling user
 * interactions, managing the environment, and visualizing the state of the
 * Q-learning model and car simulation. It contains methods to control the
 * simulation and manage the Q-learning process.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a MainWindow object.
     * 
     * Initializes the main window and its components. Sets up timers and
     * other necessary variables for simulation.
     * 
     * @param parent Optional parent widget.
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructor for the MainWindow class.
     * 
     * Cleans up resources used by the MainWindow instance.
     */
    ~MainWindow();

private slots:
    /**
     * @brief Slot for handling the play button click event.
     * 
     * Starts or resumes the simulation when the play button is clicked.
     */
    void on_playButton_clicked();

    /**
     * @brief Slot for handling the stop button click event.
     * 
     * Stops the simulation when the stop button is clicked.
     */
    void on_stopButton_clicked();

    /**
     * @brief Slot for handling the pause button click event.
     * 
     * Pauses the simulation when the pause button is clicked.
     */
    void on_pauseButton_clicked();

    /**
     * @brief Slot for handling the train button click event.
     * 
     * Initiates the training process for the Q-learning model when
     * the train button is clicked.
     */
    void on_trainButton_clicked();

    /**
     * @brief Handles the environment iteration.
     * 
     * Executes the logic for updating the environment based on the given time step.
     * 
     * @param timestep The current time step of the simulation.
     */
    void enviroment_iteration(int timestep);

    /**
     * @brief Handles the model iteration.
     * 
     * Updates the Q-learning model based on the current state and performs actions.
     */
    void model_iteration();

    /**
     * @brief Slot for loading the Q-table from a file for speed controller.
     * 
     * Loads the previously saved Q-table for the speed controller.
     */
    void on_qtable_load_sp_clicked();

    /**
     * @brief Slot for storing the Q-table to a file for speed controller.
     * 
     * Saves the current Q-table for the speed controller to a file.
     */
    void on_qtable_store_sp_clicked();

    /**
     * @brief Slot for loading the Q-table from a file for steering controller.
     * 
     * Loads the previously saved Q-table for the steering controller.
     */
    //void on_qtable_load_st_clicked();

    /**
     * @brief Slot for storing the Q-table to a file for steering controller.
     * 
     * Saves the current Q-table for the steering controller to a file.
     */
    //void on_qtable_store_st_clicked();

    /**
     * @brief Slot for reset Q-table for speed controller and steering controller
     * 
     * Reset randomly Q-table
     */
    void on_resetButton_clicked();


protected:
    /**
     * @brief Handles the paint event for custom drawing.
     * 
     * This method is overridden to perform custom drawing on the main window.
     * 
     * @param event The paint event that triggered this method.
     */
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::MainWindow *ui; ///< Pointer to the UI components of the main window.

    QPolygon car_picture; ///< A polygon representing the drawable area of the car.

    Enviroment env; ///< Enviroment state, with car state

    QPolygon env_picture; ///< The polygon representing the environment.

    int state_encoded; ///< Encoded representation of the current state.

    QTimer *timer; ///< Timer for managing the simulation updates.
    int iter; ///< Current iteration count for the simulation.
    int hit_counter; ///< Counter for hits (collisions) during the simulation.
    int success_counter; ///< Counter for successful actions during the simulation.

    int last_speed_action; ///< Last speed action .
    int last_steering_action; ///< Last steering action .
    QLearningModel controller; ///< Q-learning model.

    std::map<std::string, std::string> conf;
    int msec;
    int animation_speed;
    int time_ratio;

    std::vector<float> speed_actions;
    std::vector<float> steering_actions;

    float avg_tdr;
};

/**
 * @brief Maps a point from the simulation space to the window coordinates.
 * 
 * Converts a QPointF representing a point in the simulation space into
 * QPoint representing the corresponding point in the window coordinates.
 * 
 * @param p The point to be mapped.
 * @return The mapped point in window coordinates.
 */
QPoint map_into_window(const QPointF &p);

/**
 * @brief Maps a line from the simulation space to the window coordinates.
 * 
 * Converts a QLineF representing a line in the simulation space into
 * QLine representing the corresponding line in the window coordinates.
 * 
 * @param p The line to be mapped.
 * @return The mapped line in window coordinates.
 */
QLine map_into_window(const QLineF &p);

/**
 * @brief Maps a polygon from the simulation space to the window coordinates.
 * 
 * Converts a QPolygonF representing a polygon in the simulation space into
 * QPolygon representing the corresponding polygon in the window coordinates.
 * 
 * @param p The polygon to be mapped.
 * @return The mapped polygon in window coordinates.
 */
QPolygon map_into_window(const QPolygonF &p);

/**
 * @brief Maps a vector of lines from the simulation space to the window coordinates.
 * 
 * Converts a QVector<QLineF> representing multiple lines in the simulation space into
 * QVector<QLine> representing the corresponding lines in the window coordinates.
 * 
 * @param p The vector of lines to be mapped.
 * @return The mapped vector of lines in window coordinates.
 */
QVector<QLine> map_into_window(const QVector<QLineF> &p);

#endif // MAINWINDOW_H
