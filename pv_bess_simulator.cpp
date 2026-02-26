// ============================================================
// PV + BESS System Simulator
// Projekt zur Vorbereitung auf Praktikum bei BElectric
// Autor: Yelun Zhang | THWS BET Semester 2 | 2026
// ============================================================
// LERNZIELE (Modulhandbuch BET 2025):
//   PROG2 : OOP, Klassen, Operatoren-Überladen, Datei-I/O, Makefiles
//   GET2  : Wechselstromtechnik, komplexe Impedanz, Leistung, Drehstrom
//   IM4   : Gewöhnliche DGLs (numerisch), Fourierreihen, Laplace-Transformation
//   MT2   : Ersatzquellen, A/D-Wandler, Signalfilter, Messtechnik
// ============================================================

#include <iostream>
#include <complex>
#include <cmath>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>
#include <numeric>
#include <iomanip>

// Konstante
const double PI = 3.14159265358979323846;

// ============================================================
// TAG 1 – PROG2: Klasse PVPanel
// Modelliert ein einzelnes PV-Modul (vereinfachtes Eindioden-Modell)
// ============================================================
class PVPanel {
private:
    double V_oc;         // Leerlaufspannung [V]
    double I_sc;         // Kurzschlussstrom [A]
    double V_mpp;        // MPP-Spannung [V]
    double I_mpp;        // MPP-Strom [A]
    double area_m2;      // Modulfläche [m²]
    double eta;          // Wirkungsgrad [-]
    std::string name;    // Modulname

public:
    // Konstruktor
    PVPanel(std::string name, double V_oc, double I_sc, double V_mpp, double I_mpp, double area)
        : name(name), V_oc(V_oc), I_sc(I_sc), V_mpp(V_mpp), I_mpp(I_mpp), area_m2(area) {
        eta = (V_mpp * I_mpp) / (1000.0 * area_m2); // bei STC (1000 W/m²)
    }

    // Leistung bei gegebener Einstrahlung [W/m²] und Temperatur [°C]
    // Vereinfachtes Modell: lineare Abhängigkeit von Einstrahlung
    double getPower(double irradiance_W_m2, double temp_C = 25.0) const {
        double P_stc = V_mpp * I_mpp;  // Nennleistung bei STC
        double temp_coeff = -0.0035;    // typischer Temperaturkoeffizient [-/°C]
        double P = P_stc * (irradiance_W_m2 / 1000.0) * (1 + temp_coeff * (temp_C - 25.0));
        return std::max(0.0, P);
    }

    // Strom bei gegebener Spannung (vereinfacht linear zwischen 0 und V_oc)
    double getCurrent(double voltage, double irradiance_W_m2 = 1000.0) const {
        if (voltage >= V_oc) return 0.0;
        double I_ph = I_sc * (irradiance_W_m2 / 1000.0);  // Photostrom
        return I_ph * (1 - voltage / V_oc);                 // lineare Näherung
    }

    // Getter
    double getVoc() const { return V_oc; }
    double getIsc() const { return I_sc; }
    double getVmpp() const { return V_mpp; }
    double getImpp() const { return I_mpp; }
    double getEta() const { return eta; }
    double getPmpp() const { return V_mpp * I_mpp; }
    std::string getName() const { return name; }

    // PROG2: Operator-Überladen – Reihenschaltung von Panels
    // Ergibt neues Panel mit addierter Spannung
    PVPanel operator+(const PVPanel& other) const {
        return PVPanel(
            name + "+" + other.name,
            V_oc + other.V_oc,                         // Spannung addiert sich
            std::min(I_sc, other.I_sc),                 // Strom: kleinerer Wert
            V_mpp + other.V_mpp,
            std::min(I_mpp, other.I_mpp),
            area_m2 + other.area_m2
        );
    }

    void print() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "=== PV Panel: " << name << " ===" << std::endl;
        std::cout << "  Voc=" << V_oc << "V  Isc=" << I_sc << "A" << std::endl;
        std::cout << "  Vmpp=" << V_mpp << "V  Impp=" << I_mpp << "A  Pmpp=" << getPmpp() << "W" << std::endl;
        std::cout << "  Wirkungsgrad eta=" << eta * 100 << "%" << std::endl;
    }
};

// ============================================================
// TAG 2 – GET2: Klasse ACCircuit
// Wechselstromkreis mit komplexer Impedanz
// Lerninhalte: Zeigerdarstellung, Z = R + j*X, S = P + jQ
// ============================================================
class ACCircuit {
private:
    double R;     // Widerstand [Ohm]
    double L;     // Induktivität [H]
    double C;     // Kapazität [F] (0 = kein Kondensator)
    double f;     // Netzfrequenz [Hz]

public:
    ACCircuit(double R, double L, double C, double f = 50.0)
        : R(R), L(L), C(C), f(f) {}

    // Kreisfrequenz omega
    double omega() const { return 2 * PI * f; }

    // Komplexe Impedanz Z = R + j*(omega*L - 1/(omega*C))
    // GET2 Kerninhalt: Zeigerdarstellung mit komplexen Zahlen
    std::complex<double> getImpedance() const {
        double X_L = omega() * L;               // induktiver Blindwiderstand
        double X_C = (C > 0) ? 1.0 / (omega() * C) : 0.0;  // kapazitiver Blindwiderstand
        return std::complex<double>(R, X_L - X_C);
    }

    // Strom bei gegebener Spannung (komplex)
    std::complex<double> getCurrent(std::complex<double> U) const {
        return U / getImpedance();
    }

    // Scheinleistung S = U * I* (konjugiert)
    // P = Re(S) [W], Q = Im(S) [VAr], |S| = Scheinleistung [VA]
    std::complex<double> getApparentPower(std::complex<double> U) const {
        std::complex<double> I = getCurrent(U);
        return U * std::conj(I);
    }

    double getPower(double U_rms) const {
        std::complex<double> U(U_rms, 0);
        return std::real(getApparentPower(U));  // Wirkleistung
    }

    double getReactivePower(double U_rms) const {
        std::complex<double> U(U_rms, 0);
        return std::imag(getApparentPower(U));  // Blindleistung
    }

    double getPowerFactor(double U_rms) const {
        std::complex<double> U(U_rms, 0);
        std::complex<double> S = getApparentPower(U);
        return std::real(S) / std::abs(S);  // cos(phi)
    }

    // Frequenzgang |Z(omega)| – GET2: grafische Darstellung
    std::vector<std::pair<double, double>> getFrequencyResponse(double f_min, double f_max, int N) const {
        std::vector<std::pair<double, double>> result;
        for (int i = 0; i < N; i++) {
            double fi = f_min * std::pow(f_max / f_min, (double)i / (N - 1));
            double omega_i = 2 * PI * fi;
            double X_L = omega_i * L;
            double X_C = (C > 0) ? 1.0 / (omega_i * C) : 0.0;
            double Z_mag = std::sqrt(R * R + (X_L - X_C) * (X_L - X_C));
            result.push_back({fi, Z_mag});
        }
        return result;
    }

    void print(double U_rms = 230.0) const {
        std::complex<double> Z = getImpedance();
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "=== AC-Kreis bei f=" << f << "Hz ===" << std::endl;
        std::cout << "  Z = " << R << " + j*" << std::imag(Z) << " Ohm" << std::endl;
        std::cout << "  |Z| = " << std::abs(Z) << " Ohm" << std::endl;
        std::cout << "  P = " << getPower(U_rms) << " W" << std::endl;
        std::cout << "  Q = " << getReactivePower(U_rms) << " VAr" << std::endl;
        std::cout << "  cos(phi) = " << getPowerFactor(U_rms) << std::endl;
    }
};

// ============================================================
// TAG 3 – MT2: Klasse Battery (Thevenin-Ersatzschaltbild)
// Lerninhalte: Ersatzquellen, Operationsverstärker (Grundschaltungen)
// BElectric: Basis jedes BESS-Projekts
// ============================================================
class Battery {
private:
    double capacity_Wh;   // Nennkapazität [Wh]
    double V_nom;         // Nennspannung [V]
    double R_int;         // Innenwiderstand [Ohm] (Thevenin)
    double soc;           // State of Charge [0..1]
    double soc_min;       // Minimaler SoC [-]
    double soc_max;       // Maximaler SoC [-]
    std::string chem;     // Chemie (z.B. "LiFePO4")

    // Interne Spannung als Funktion des SoC (vereinfacht linear)
    double getOCV() const {
        double V_min = V_nom * 0.90;
        double V_max = V_nom * 1.05;
        return V_min + (V_max - V_min) * soc;
    }

public:
    Battery(double capacity_Wh, double V_nom, double R_int = 0.05, 
            double soc_init = 0.5, std::string chem = "LiFePO4")
        : capacity_Wh(capacity_Wh), V_nom(V_nom), R_int(R_int),
          soc(soc_init), soc_min(0.1), soc_max(0.9), chem(chem) {}

    // Klemmenspannung beim Laden/Entladen
    // MT2: Thevenin-Ersatzschaltbild: U_klemme = U_OCV ± I * R_i
    double getTerminalVoltage(double current_A) const {
        return getOCV() - current_A * R_int;  // positiv = Entladung
    }

    // Laden/Entladen über Zeitschritt dt [s]
    // Gibt tatsächlich geladene/entladene Energie zurück [Wh]
    double charge(double power_W, double dt_s) {
        if (power_W > 0 && soc >= soc_max) return 0.0;  // voll
        if (power_W < 0 && soc <= soc_min) return 0.0;  // leer

        double energy_Wh = power_W * dt_s / 3600.0;
        double new_soc = soc + energy_Wh / capacity_Wh;
        new_soc = std::max(soc_min, std::min(soc_max, new_soc));
        double actual_energy = (new_soc - soc) * capacity_Wh;
        soc = new_soc;
        return actual_energy;
    }

    double getSoC() const { return soc; }
    double getCapacity() const { return capacity_Wh; }
    double getStoredEnergy() const { return soc * capacity_Wh; }

    void print() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "=== Batterie (" << chem << ") ===" << std::endl;
        std::cout << "  Kapazitaet: " << capacity_Wh << " Wh" << std::endl;
        std::cout << "  SoC: " << soc * 100 << "%" << std::endl;
        std::cout << "  OCV: " << getOCV() << " V" << std::endl;
        std::cout << "  Gespeicherte Energie: " << getStoredEnergy() << " Wh" << std::endl;
    }
};

// ============================================================
// TAG 4 – IM4: Numerische DGL-Lösung (Euler-Methode)
// Anwendung: Ladevorgang eines Kondensators (RC-Kreis)
// DGL: C * du/dt = (U_0 - u) / R  →  u(t) = U_0 * (1 - e^(-t/RC))
// ============================================================
class ODESolver {
public:
    // Einfacher Euler-Schritt: y_{n+1} = y_n + h * f(t_n, y_n)
    // f(t, y) ist die rechte Seite der DGL dy/dt = f(t,y)
    static std::vector<std::pair<double,double>> eulerSolve(
        std::function<double(double, double)> f,
        double y0, double t_start, double t_end, double dt)
    {
        std::vector<std::pair<double,double>> result;
        double t = t_start, y = y0;
        while (t <= t_end) {
            result.push_back({t, y});
            y += dt * f(t, y);
            t += dt;
        }
        return result;
    }

    // RC-Ladevorgang: du/dt = (U0 - u) / (R*C)
    // Analytische Lösung: u(t) = U0 * (1 - exp(-t / tau))
    static double rcAnalytical(double t, double U0, double tau) {
        return U0 * (1.0 - std::exp(-t / tau));
    }
};

// ============================================================
// TAG 5 – MT2 + IM4: Signalfilter & Fourierreihe
// MT2: FIR-Filter (Moving Average) für Irradiance-Glättung
// IM4: Fourierreihen-Koeffizienten berechnen
// ============================================================
class SignalProcessor {
public:
    // Moving Average Filter (FIR) – MT2: digitale Filter
    static std::vector<double> movingAverage(const std::vector<double>& signal, int window) {
        std::vector<double> filtered(signal.size(), 0.0);
        for (size_t i = 0; i < signal.size(); i++) {
            int start = std::max(0, (int)i - window / 2);
            int end = std::min((int)signal.size() - 1, (int)i + window / 2);
            double sum = 0;
            for (int j = start; j <= end; j++) sum += signal[j];
            filtered[i] = sum / (end - start + 1);
        }
        return filtered;
    }

    // Fourierreihen-Koeffizienten a_n, b_n – IM4: Fourierreihen
    // Berechnet die ersten N Koeffizienten für ein tagesperiodisches Signal
    static void fourierCoefficients(const std::vector<double>& signal, int N,
                                    std::vector<double>& a, std::vector<double>& b) {
        int M = signal.size();
        a.resize(N + 1, 0.0);
        b.resize(N + 1, 0.0);
        a[0] = std::accumulate(signal.begin(), signal.end(), 0.0) / M;
        for (int n = 1; n <= N; n++) {
            for (int k = 0; k < M; k++) {
                double t = 2 * PI * n * k / M;
                a[n] += signal[k] * std::cos(t);
                b[n] += signal[k] * std::sin(t);
            }
            a[n] *= 2.0 / M;
            b[n] *= 2.0 / M;
        }
    }
};

// ============================================================
// TAG 8 – IM4: Laplace-Transformation (Anwendung)
// Übertragungsfunktion H(s) eines RC-Tiefpassfilters
// H(s) = 1 / (1 + s*R*C)  →  Grenzfrequenz f_g = 1/(2*pi*RC)
// ============================================================
class LaplaceFilter {
private:
    double R, C;
public:
    LaplaceFilter(double R, double C) : R(R), C(C) {}
    double tau() const { return R * C; }
    double cutoffFreq() const { return 1.0 / (2 * PI * R * C); }

    // Frequenzgang H(jω) = 1 / (1 + jωRC)
    std::complex<double> H(double freq) const {
        std::complex<double> s(0, 2 * PI * freq);  // s = jω
        return 1.0 / (1.0 + s * R * C);
    }

    double getMagnitude(double freq) const { return std::abs(H(freq)); }
    double getMagnitudedB(double freq) const { return 20 * std::log10(getMagnitude(freq)); }
    double getPhase(double freq) const { return std::arg(H(freq)) * 180 / PI; }

    void printBode(double f_start = 0.01, double f_end = 10000, int N = 50) const {
        std::cout << "=== Bode-Diagramm RC-Tiefpass ===" << std::endl;
        std::cout << "  tau = " << tau() * 1000 << " ms" << std::endl;
        std::cout << "  f_g = " << cutoffFreq() << " Hz" << std::endl;
        std::cout << std::setw(12) << "f [Hz]" << std::setw(14) << "|H| [dB]" << std::setw(14) << "Phase [deg]" << std::endl;
        for (int i = 0; i < N; i += N / 8) {
            double f = f_start * std::pow(f_end / f_start, (double)i / (N - 1));
            std::cout << std::setw(12) << std::fixed << std::setprecision(2) << f
                      << std::setw(14) << getMagnitudedB(f)
                      << std::setw(14) << getPhase(f) << std::endl;
        }
    }
};

// ============================================================
// TAG 9 – GET2: Drehstromsystem
// Leistung im symmetrischen Dreiphasensystem
// ============================================================
class ThreePhaseGrid {
private:
    double V_line;  // verkettete Spannung [V]
    double f;       // Netzfrequenz [Hz]

public:
    ThreePhaseGrid(double V_line = 400.0, double f = 50.0) : V_line(V_line), f(f) {}

    double V_phase() const { return V_line / std::sqrt(3.0); }

    // Wirkleistung P = sqrt(3) * U_L * I * cos(phi) – GET2: Drehstromleistung
    double activePower(double I, double cos_phi) const {
        return std::sqrt(3.0) * V_line * I * cos_phi;
    }

    // Blindleistung Q = sqrt(3) * U_L * I * sin(phi)
    double reactivePower(double I, double cos_phi) const {
        double sin_phi = std::sqrt(1.0 - cos_phi * cos_phi);
        return std::sqrt(3.0) * V_line * I * sin_phi;
    }

    // Strom aus Leistung berechnen (für Wechselrichter-Auslegung)
    double currentFromPower(double P_W, double cos_phi = 0.95) const {
        return P_W / (std::sqrt(3.0) * V_line * cos_phi);
    }

    void print(double P_W = 10000, double cos_phi = 0.95) const {
        std::cout << "=== Drehstromnetz ===" << std::endl;
        std::cout << "  U_L = " << V_line << " V (verkettet), U_Ph = " << V_phase() << " V" << std::endl;
        std::cout << "  P = " << P_W / 1000 << " kW, cos(phi) = " << cos_phi << std::endl;
        std::cout << "  I = " << currentFromPower(P_W, cos_phi) << " A" << std::endl;
        std::cout << "  Q = " << reactivePower(currentFromPower(P_W, cos_phi), cos_phi) / 1000 << " kVAr" << std::endl;
    }
};

// ============================================================
// TAG 11 – MT2: A/D-Wandler Simulation
// ============================================================
class ADConverter {
private:
    int bits;        // Auflösung
    double V_ref;    // Referenzspannung [V]

public:
    ADConverter(int bits = 12, double V_ref = 3.3) : bits(bits), V_ref(V_ref) {}

    int quantize(double voltage) const {
        int levels = 1 << bits;  // 2^bits
        double normalized = std::max(0.0, std::min(1.0, voltage / V_ref));
        return static_cast<int>(normalized * (levels - 1));
    }

    double reconstruct(int code) const {
        return code * V_ref / ((1 << bits) - 1);
    }

    // Quantisierungsrauschen: SNR = 6.02*N + 1.76 dB (Daumenregel)
    double theoreticalSNR() const { return 6.02 * bits + 1.76; }

    void print() const {
        std::cout << "=== A/D-Wandler ===" << std::endl;
        std::cout << "  Aufloesung: " << bits << " Bit → " << (1 << bits) << " Stufen" << std::endl;
        std::cout << "  LSB = " << V_ref / (1 << bits) * 1000 << " mV" << std::endl;
        std::cout << "  Theoret. SNR = " << theoreticalSNR() << " dB" << std::endl;
    }
};

// ============================================================
// Hauptprogramm – Demo aller Klassen
// ============================================================
int main() {
    std::cout << "============================================" << std::endl;
    std::cout << " PV + BESS Simulator – THWS BET Semester 2" << std::endl;
    std::cout << " Praktikum-Vorbereitung: BElectric" << std::endl;
    std::cout << "============================================" << std::endl << std::endl;

    // --- TAG 1: PV Panel ---
    PVPanel panel1("SunPower_400W", 48.5, 10.2, 41.0, 9.8, 1.7);
    PVPanel panel2("SunPower_400W", 48.5, 10.2, 41.0, 9.8, 1.7);
    panel1.print();
    std::cout << "  Leistung bei 800 W/m², 35°C: " 
              << panel1.getPower(800, 35) << " W" << std::endl;
    
    // Reihenschaltung (PROG2: Operator-Überladen)
    PVPanel string = panel1 + panel2;
    std::cout << "\n  Reihenschaltung (2 Module):" << std::endl;
    std::cout << "  Vmpp = " << string.getVmpp() << " V, Pmpp = " << string.getPmpp() << " W" << std::endl;

    // --- TAG 2: AC-Kreis (GET2) ---
    std::cout << std::endl;
    ACCircuit inverter_filter(0.5, 0.001, 47e-6);  // LCL-Filter eines Wechselrichters
    inverter_filter.print(230.0);

    // --- TAG 3: Batterie (MT2) ---
    std::cout << std::endl;
    Battery bess(10000, 48.0, 0.02, 0.5, "LiFePO4");  // 10 kWh BESS
    bess.print();
    bess.charge(5000, 3600);  // 5 kW laden, 1 Stunde
    std::cout << "  Nach 1h Laden mit 5kW:" << std::endl;
    bess.print();

    // --- TAG 4: RC-Ladevorgang (IM4) ---
    std::cout << "\n=== RC-Ladevorgang (IM4: DGL numerisch) ===" << std::endl;
    double R = 100.0, C_val = 0.001, U0 = 48.0;
    double tau = R * C_val;
    auto rc_ode = [&](double t, double u) { return (U0 - u) / (R * C_val); };
    auto result = ODESolver::eulerSolve(rc_ode, 0.0, 0.0, 5 * tau, tau / 100);
    std::cout << "  tau = " << tau * 1000 << " ms" << std::endl;
    std::cout << "  t=tau:  Euler=" << result[100].second << " V, Analytisch=" 
              << ODESolver::rcAnalytical(tau, U0, tau) << " V" << std::endl;
    std::cout << "  t=5tau: Euler=" << result[result.size()-1].second << " V (soll ~" << U0 << " V)" << std::endl;

    // --- TAG 8: Laplace / Bode (IM4) ---
    std::cout << std::endl;
    LaplaceFilter lpf(1000, 1e-6);  // 1kΩ, 1µF → f_g ≈ 159 Hz
    lpf.printBode();

    // --- TAG 9: Drehstrom (GET2) ---
    std::cout << std::endl;
    ThreePhaseGrid grid(400.0);
    grid.print(50000, 0.95);  // 50 kW PV-Anlage

    // --- TAG 11: ADC (MT2) ---
    std::cout << std::endl;
    ADConverter adc(12, 3.3);
    adc.print();
    double test_V = 1.65;  // Messwert
    int code = adc.quantize(test_V);
    double reconstructed = adc.reconstruct(code);
    std::cout << "  Messwert: " << test_V << " V → Code: " << code 
              << " → Rekonstruiert: " << reconstructed << " V"
              << " (Fehler: " << (reconstructed - test_V) * 1000 << " mV)" << std::endl;

    // --- CSV-Output für spätere Visualisierung ---
    std::ofstream csv("simulation_output.csv");
    csv << "time_s,u_RC_V,soc_pct,pv_power_W\n";
    Battery sim_batt(10000, 48.0, 0.02, 0.3);
    for (int t = 0; t < 86400; t += 3600) {
        double hour = t / 3600.0;
        // Vereinfachtes Tagesprofil der Einstrahlung (Sinusform)
        double irr = std::max(0.0, 900 * std::sin(PI * (hour - 6) / 12.0)) * (hour >= 6 && hour <= 18 ? 1 : 0);
        double pv = panel1.getPower(irr, 25 + irr / 100);
        sim_batt.charge(pv * 0.5, 3600);  // 50% in Batterie
        double u_rc = ODESolver::rcAnalytical(t, U0, tau);
        csv << t << "," << u_rc << "," << sim_batt.getSoC() * 100 << "," << pv << "\n";
    }
    csv.close();
    std::cout << "\n  CSV gespeichert: simulation_output.csv" << std::endl;

    std::cout << "\n============================================" << std::endl;
    std::cout << " Naechster Schritt: Makefile erstellen," << std::endl;
    std::cout << " auf GitHub pushen, README.md schreiben!" << std::endl;
    std::cout << "============================================" << std::endl;

    return 0;
}
