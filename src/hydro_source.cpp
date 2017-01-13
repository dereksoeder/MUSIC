// Copyright 2017 Chun Shen

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include "./hydro_source.h"

using namespace std;

hydro_source::hydro_source(InitData *DATA_in) {
    DATA_ptr = DATA_in;
    if (DATA_ptr->Initial_profile == 12) {
        sigma_tau = 0.2;
        sigma_x = 0.5;
        sigma_eta = 0.5;
        volume = DATA_ptr->delta_x*DATA_ptr->delta_y*DATA_ptr->delta_eta;
        read_in_QCD_strings_and_partons();
    }

}

hydro_source::~hydro_source() {
    if (DATA_ptr->Initial_profile == 12) {
        QCD_strings_list.clear();
        parton_list.clear();
    }
}

void hydro_source::read_in_QCD_strings_and_partons() {
    string QCD_strings_filename = DATA_ptr->initName;
    string partons_filename = DATA_ptr->initName_rhob;
    cout << "read in QCD strings list from " << QCD_strings_filename
         << " and partons list from " << partons_filename << endl;
    string text_string;

    ifstream QCD_strings_file(QCD_strings_filename.c_str());
    if (!QCD_strings_file) {
        cerr << "Error:hydro_source::read_in_QCD_strings_and_partons: "
             << "can not open QCD strings file: " << QCD_strings_filename
             << endl;
        exit(1);
    }
    getline(QCD_strings_file, text_string);  // read the header
    // now we read in data
    getline(QCD_strings_file, text_string);
    while (!QCD_strings_file.eof()) {
        stringstream text_stream(text_string);
        QCD_string new_string;
        text_stream >> new_string.norm >> new_string.delta_E
                    >> new_string.tau_form
                    >> new_string.x_perp >> new_string.y_perp
                    >> new_string.eta_s_left >> new_string.eta_s_right
                    >> new_string.y_l >> new_string.y_r;
        QCD_strings_list.push_back(new_string);
        getline(QCD_strings_file, text_string);
    }
    QCD_strings_file.close();
    
    ifstream partons_file(partons_filename.c_str());
    if (!partons_file) {
        cerr << "Error:hydro_source::read_in_QCD_strings_and_partons: "
             << "can not open parton list file: " << partons_filename
             << endl;
        exit(1);
    }
    getline(partons_file, text_string);      // read the header
    // now we read in data
    getline(partons_file, text_string);
    while (!partons_file.eof()) {
        stringstream text_stream(text_string);
        parton new_parton;
        text_stream >> new_parton.tau >> new_parton.x >> new_parton.y
                    >> new_parton.eta_s >> new_parton.rapidity;
        new_parton.baryon_number = 1.0;
        parton_list.push_back(new_parton);
        getline(partons_file, text_string);
    }
    partons_file.close();
}

void hydro_source::get_hydro_energy_source(
    double tau, double x, double y, double eta_s, double *u_mu, double *j_mu) {
    // clean up j_mu
    for (int i = 0; i < 3; i++) {
        j_mu[i] = 0.0;
    }
    double ed = 0.;
    if (DATA_ptr->Initial_profile == 12) {
        double n_sigma_skip = 5.;
        for (vector<QCD_string>::iterator it = QCD_strings_list.begin();
             it != QCD_strings_list.end(); it++) {
            // skip the evaluation if the strings is too far away in the
            // space-time grid
            double tau_dis = tau - (*it).tau_form;
            if (fabs(tau_dis) > n_sigma_skip*sigma_tau) {
                continue;
            }
            double x_dis = x - (*it).x_perp;
            if (fabs(x_dis) > n_sigma_skip*sigma_x) {
                continue;
            }
            double y_dis = y - (*it).y_perp;
            if (fabs(y_dis) > n_sigma_skip*sigma_x) {
                continue;
            }
            if (eta_s < (*it).eta_s_left - n_sigma_skip*sigma_eta
                 || eta_s > (*it).eta_s_right + n_sigma_skip*sigma_eta) {
                continue;
            }
            double exp_tau = exp(-tau_dis*tau_dis/(sigma_tau*sigma_tau));
            double exp_xperp = exp(-(x_dis*x_dis + y_dis*y_dis)
                                    /(sigma_x*sigma_x));
            double exp_eta_s = 1.;
            if (eta_s > (*it).eta_s_left && eta_s < (*it).eta_s_right) {
                exp_eta_s = 1.;
            } else if (eta_s < (*it).eta_s_left) {
                double eta_s_dis = eta_s - (*it).eta_s_left;
                exp_eta_s = exp(-(eta_s_dis*eta_s_dis)/(sigma_eta*sigma_eta));
            } else {
                double eta_s_dis = eta_s - (*it).eta_s_right;
                exp_eta_s = exp(-(eta_s_dis*eta_s_dis)/(sigma_eta*sigma_eta));
            }
            double e_local = exp_tau*exp_xperp*exp_eta_s;
            // double y_interp = (
            //         (*it).y_l + ((*it).y_r - (*it).y_l)
            //                     /((*it).eta_s_right - (*it).eta_s_left)
            //                     *(eta_s - (*it).eta_s_left));
            ed += e_local;
        }
        j_mu[0] = ed*u_mu[0]*volume;
        j_mu[1] = ed*u_mu[1]*volume;
        j_mu[2] = ed*u_mu[2]*volume;
        j_mu[3] = ed*u_mu[3]*volume;
    }
}

double hydro_source::get_hydro_rhob_source(double tau, double x, double y,
                                           double eta_s) {
    double res = 0.;
    if (DATA_ptr->Initial_profile == 12) {
        double n_sigma_skip = 5.;
        for (vector<parton>::iterator it = parton_list.begin();
             it != parton_list.end(); it++) {
            // skip the evaluation if the strings is too far away in the
            // space-time grid
            double tau_dis = tau - (*it).tau;
            if (fabs(tau_dis) > n_sigma_skip*sigma_tau) {
                continue;
            }
            double x_dis = x - (*it).x;
            if (fabs(x_dis) > n_sigma_skip*sigma_x) {
                continue;
            }
            double y_dis = y - (*it).y;
            if (fabs(y_dis) > n_sigma_skip*sigma_x) {
                continue;
            }
            double eta_s_dis = eta_s - (*it).eta_s;
            if (fabs(eta_s_dis) > n_sigma_skip*sigma_eta) {
                continue;
            }
            double exp_tau = exp(-tau_dis*tau_dis/(sigma_tau*sigma_tau));
            double exp_xperp = exp(-(x_dis*x_dis + y_dis*y_dis)
                                    /(sigma_x*sigma_x));
            double exp_eta_s = exp(-eta_s_dis*eta_s_dis/(sigma_eta*sigma_eta));
            res += exp_tau*exp_xperp*exp_eta_s;
        }
    }
    res *= volume;
    return(res);
}

void hydro_source::get_hydro_energy_source_before_tau(
    double tau, double x, double y, double eta_s, double *j_mu) {
    double *u = new double[4];
    double *j_mu_one_step = new double[4];
    for (int i = 0; i < 3; i++) {
        j_mu[i] = 0.0;  // clean up j_mu
        u[i] = 0.0;
    }
    u[0] = 1.0;
    double tau0 = 0.0;
    double dtau = DATA_ptr->delta_tau;
    int n_tau_steps = static_cast<int>((tau - tau0)/dtau);
    for (int i = 0; i < n_tau_steps; i++) {
        for (int j = 0; j < 3; j++) {
            j_mu_one_step[j] = 0.0;
        }
        double tau_local = tau0 + i*dtau;
        get_hydro_energy_source(tau_local, x, y, eta_s, u, j_mu_one_step);
        for (int j = 0; j < 3; j++) {
            j_mu[j] += j_mu_one_step[j]*dtau;
        }
    }
    delete[] u;
    delete[] j_mu_one_step;
}

double hydro_source::get_hydro_rhob_source_before_tau(
        double tau, double x, double y, double eta_s) {
    double res = 0.;
    double tau0 = 0.0;
    double dtau = DATA_ptr->delta_tau;
    int n_tau_steps = static_cast<int>((tau - tau0)/dtau);
    for (int i = 0; i < n_tau_steps; i++) {
        double res_local = 0.0;
        double tau_local = tau0 + i*dtau;
        res_local = get_hydro_rhob_source(tau_local, x, y, eta_s);
        res += res_local*dtau;
    }
    return(res);
}
