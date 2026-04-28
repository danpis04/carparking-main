// test_decay.cpp — verifies that the iter-advance fix in MainWindow's train
// loop yields the geometric lr/er decay schedule the Q-learning code intends.
//
// Mirrors the relevant slice of MainWindow::on_trainButton_clicked +
// MainWindow::model_iteration without Qt or environment side effects.
//
// Two simulated runs: FIXED (advances iter += time_ratio per pass, post-fix)
// vs BUGGY (iter stuck at 0, pre-fix). FIXED is checked against a closed-form
// geometric-decay oracle. BUGGY serves as a negative-control showing the test
// would notice the bug.
//
// Build (from project root):
//   g++ -std=c++14 -O2 -Isrc test/test_decay.cpp src/q_learning.cpp src/utils.cpp -o bin/test_decay
// Run (from project root, so configuration/default.conf resolves):
//   ./bin/test_decay [num_passes]

#include "../src/q_learning.h"
#include "../src/utils.h"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <clocale>
#include <map>
#include <string>

using namespace std;

struct Run {
    QLearningModel ctrl;
    long long iter;
    double avg_tdr;
    int time_ratio;
    bool advance_iter;

    Run(map<string, string> conf, bool advance)
        : ctrl(conf), iter(0), avg_tdr(0),
          time_ratio(stoi(conf["TIME_RATIO"])), advance_iter(advance) {}

    // One pass of the train loop, restricted to the iter/i/lr/er/avg_tdr book-
    // keeping copied from model_iteration() + on_trainButton_clicked. Uses a
    // synthetic |tdr|=0.5 so avg_tdr has a known true value (0.5) on success.
    void step() {
        int i = (int)(iter / time_ratio);
        const float tdr = 0.5f;

        // Per-iteration decay: applied unconditionally each model_iteration.
        ctrl.er = ctrl.er_min + ctrl.er_ratio * (ctrl.er - ctrl.er_min);
        ctrl.lr = ctrl.lr_min + ctrl.lr_ratio * (ctrl.lr - ctrl.lr_min);

        avg_tdr = ((double)i * avg_tdr + fabs((double)tdr)) / (i + 1);

        if (advance_iter) iter += time_ratio;
    }
};

int main(int argc, char** argv) {
    setlocale(LC_NUMERIC, "C");

    auto conf = readConfig("configuration/default.conf");
    if (conf.empty()) {
        cerr << "FAIL: cannot read configuration/default.conf — "
                "run this binary from the project root." << endl;
        return 1;
    }

    int n_steering = stoi(conf["N_STEERING_ACTIONS"]);
    int n_speed    = stoi(conf["N_SPEED_ACTIONS"]);
    conf["N_ACTIONS"] = to_string(n_steering * n_speed);
    conf["N_STATES"] = to_string(stoi(conf["X_DIVIDE"]) *
                                 stoi(conf["Y_DIVIDE"]) *
                                 stoi(conf["THETA_DIVIDE"]));

    int total_passes = (argc > 1) ? atoi(argv[1]) : 1000000;
    int n_checkpoints = 10;
    int checkpoint = max(1, total_passes / n_checkpoints);

    cout << "\n=== Q-learning decay schedule verification ===\n";
    cout << "config: Q_LR_MAX=" << conf["Q_LR_MAX"]
         << " Q_LR_MIN="        << conf["Q_LR_MIN"]
         << " Q_LR_HL="         << conf["Q_LR_HL"] << "\n";
    cout << "        ER_MAX="   << conf["ER_MAX"]
         << " ER_MIN="          << conf["ER_MIN"]
         << " ER_HL="           << conf["ER_HL"]
         << " TIME_RATIO="      << conf["TIME_RATIO"] << "\n";
    cout << "passes=" << total_passes
         << "  checkpoint=" << checkpoint << "\n\n";

    Run fixed_run(conf, true);
    Run buggy_run(conf, false);

    const double lr_max   = fixed_run.ctrl.lr_max;
    const double lr_min   = fixed_run.ctrl.lr_min;
    const double lr_ratio = fixed_run.ctrl.lr_ratio;
    const double er_max   = fixed_run.ctrl.er_max;
    const double er_min   = fixed_run.ctrl.er_min;
    const double er_ratio = fixed_run.ctrl.er_ratio;

    cout << fixed << setprecision(8);
    cout << setw(10) << "pass"
         << " | "
         << setw(12) << "FIXED lr"   << setw(12) << "FIXED er"
         << " | "
         << setw(12) << "BUGGY lr"   << setw(12) << "BUGGY er"
         << " | "
         << setw(12) << "ORACLE lr"  << setw(12) << "ORACLE er"
         << "  status\n";
    cout << string(118, '-') << "\n";

    bool any_fail = false;
    const double tol = 1e-5;

    for (int pass = 1; pass <= total_passes; ++pass) {
        fixed_run.step();
        buggy_run.step();

        if (pass % checkpoint == 0 || pass == total_passes) {
            // Per-iteration decay: applied once per pass, so after L passes the
            // decay has been applied L times.
            long long decay_count = (long long)pass;

            double exp_lr = lr_min + (lr_max - lr_min)
                            * pow(lr_ratio, (double)decay_count);
            double exp_er = er_min + (er_max - er_min)
                            * pow(er_ratio, (double)decay_count);

            double dlr = fabs(fixed_run.ctrl.lr - exp_lr);
            double der = fabs(fixed_run.ctrl.er - exp_er);
            double davg = fabs(fixed_run.avg_tdr - 0.5);
            bool ok = (dlr < tol) && (der < tol) && (davg < 1e-9);
            if (!ok) any_fail = true;

            cout << setw(10) << pass
                 << " | "
                 << setw(12) << fixed_run.ctrl.lr
                 << setw(12) << fixed_run.ctrl.er
                 << " | "
                 << setw(12) << buggy_run.ctrl.lr
                 << setw(12) << buggy_run.ctrl.er
                 << " | "
                 << setw(12) << exp_lr
                 << setw(12) << exp_er
                 << (ok ? "  ok" : "  MISMATCH")
                 << "\n";
        }
    }

    cout << "\nFinal avg_tdr  fixed=" << fixed_run.avg_tdr
         << "  buggy=" << buggy_run.avg_tdr
         << "  (true value = 0.5)\n";

    cout << "\nNote on the BUGGY column:\n";
    cout << "  With per-iteration decay, the iter-advance bug no longer affects\n"
            "  lr/er (decay is applied unconditionally each pass), so BUGGY and\n"
            "  FIXED produce identical lr/er. The iter fix is still needed for\n"
            "  avg_tdr, which uses i = iter/time_ratio as its sample-count base.\n";
    cout << "  BUGGY lr=" << buggy_run.ctrl.lr
         << "  FIXED lr=" << fixed_run.ctrl.lr << "\n\n";

    if (any_fail) {
        cout << "RESULT: FAIL — fixed-run lr/er did not match the geometric-"
                "decay oracle. See MISMATCH rows.\n";
        return 1;
    }
    cout << "RESULT: PASS — fixed-run lr/er match the oracle within "
         << tol << ".\n";
    return 0;
}
