#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <iostream>
#include <functional>

class ExpectileRiskManager {
private:
    std::vector<double> returns;
    std::vector<double> volatility;
    std::vector<double> vix_data;
    
    // Normal CDF approximation using Abramowitz and Stegun
    double normalCDF(double x) const {
        return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
    }
    
    // Inverse normal CDF approximation
    double inverseNormalCDF(double p) const {
        if (p <= 0.0) return -INFINITY;
        if (p >= 1.0) return INFINITY;
        
        // Beasley-Springer-Moro algorithm approximation
        double a0 = 2.50662823884;
        double a1 = -18.61500062529;
        double a2 = 41.39119773534;
        double a3 = -25.44106049637;
        double b1 = -8.47351093090;
        double b2 = 23.08336743743;
        double b3 = -21.06224101826;
        double b4 = 3.13082909833;
        double c0 = -2.78718931138;
        double c1 = -2.29796479134;
        double c2 = 4.85014127135;
        double c3 = 2.32121276858;
        double d1 = 3.54388924762;
        double d2 = 1.63706781897;
        
        double u = p - 0.5;
        double r;
        
        if (std::abs(u) < 0.42) {
            r = u * u;
            return u * (((a3 * r + a2) * r + a1) * r + a0) / 
                   ((((b4 * r + b3) * r + b2) * r + b1) * r + 1.0);
        } else {
            r = p;
            if (u > 0.0) r = 1.0 - p;
            r = std::log(-std::log(r));
            r = c0 + r * (c1 + r * (c2 + r * c3)) / 
                (1.0 + r * (d1 + r * d2));
            if (u < 0.0) r = -r;
            return r;
        }
    }

public:
    ExpectileRiskManager() = default;
    
    // Set market data
    void setReturns(const std::vector<double>& ret) { returns = ret; }
    void setVolatility(const std::vector<double>& vol) { volatility = vol; }
    void setVIXData(const std::vector<double>& vix) { vix_data = vix; }
    
    // Core expectile calculation using asymmetric least squares
    // Formula (2) from paper: E_τ(X) = arg min E[|τ - 1{X≤m}|(X - m)²]
    double calculateExpectile(const std::vector<double>& data, double tau) const {
        if (data.empty() || tau <= 0.0 || tau >= 1.0) {
            throw std::invalid_argument("Invalid data or tau parameter");
        }
        
        double expectile = 0.0;
        double prev_expectile = 0.0;
        const double tolerance = 1e-8;
        const int max_iterations = 1000;
        
        // Initial guess - use quantile approximation
        std::vector<double> sorted_data = data;
        std::sort(sorted_data.begin(), sorted_data.end());
        expectile = sorted_data[static_cast<size_t>(tau * sorted_data.size())];
        
        // Newton-Raphson iteration
        for (int iter = 0; iter < max_iterations; ++iter) {
            double sum_weights = 0.0;
            double weighted_sum = 0.0;
            
            for (double x : data) {
                double weight = (x <= expectile) ? tau : (1.0 - tau);
                sum_weights += weight;
                weighted_sum += weight * x;
            }
            
            prev_expectile = expectile;
            expectile = weighted_sum / sum_weights;
            
            if (std::abs(expectile - prev_expectile) < tolerance) {
                break;
            }
        }
        
        return expectile;
    }
    
    // Dynamic expectile level calculation
    // Formula (13): τ_t = Φ(δ₀ + δ₁τ_{t-1} + δ₂σ²_{t-1} + δ₃|r_{t-1}|)
    double calculateDynamicExpectileLevel(double prev_tau, double prev_volatility, 
                                        double prev_return, 
                                        const std::vector<double>& delta_params) const {
        if (delta_params.size() != 4) {
            throw std::invalid_argument("Delta parameters must have 4 elements");
        }
        
        double linear_combination = delta_params[0] + 
                                   delta_params[1] * prev_tau + 
                                   delta_params[2] * (prev_volatility * prev_volatility) + 
                                   delta_params[3] * std::abs(prev_return);
        
        return normalCDF(linear_combination);
    }
    
    // Multivariate expectile framework for portfolio-level risk assessment
    // Formula (14): E_τ[r_t|F_{t-1}] = α_τ + Σ B_{τ,i}r_{t-i} + Σ Γ_{τ,j}z_{t-j}
    std::vector<double> multivariateExpectileFramework(
        const std::vector<std::vector<double>>& lagged_returns_matrix,
        const std::vector<std::vector<double>>& additional_variables_matrix,
        const std::vector<double>& alpha_tau,
        const std::vector<std::vector<double>>& B_tau_matrices,
        const std::vector<std::vector<double>>& Gamma_tau_matrices) const {
        
        size_t num_assets = lagged_returns_matrix.size();
        std::vector<double> expectile_values(num_assets);
        
        for (size_t asset = 0; asset < num_assets; ++asset) {
            double expectile = alpha_tau[asset];
            
            // Add B_{τ,i}r_{t-i} terms
            for (size_t i = 0; i < lagged_returns_matrix[asset].size() && i < B_tau_matrices[asset].size(); ++i) {
                expectile += B_tau_matrices[asset][i] * lagged_returns_matrix[asset][i];
            }
            
            // Add Γ_{τ,j}z_{t-j} terms
            for (size_t j = 0; j < additional_variables_matrix[asset].size() && j < Gamma_tau_matrices[asset].size(); ++j) {
                expectile += Gamma_tau_matrices[asset][j] * additional_variables_matrix[asset][j];
            }
            
            expectile_values[asset] = expectile;
        }
        
        return expectile_values;
    }
    
    // Bayesian prior distributions for expectile model parameters
    // Formula (15): β_τ ~ N(0, σ²_β I)
    std::vector<double> generateBayesianBetaPrior(size_t dimension, double sigma_beta_squared, 
                                                std::mt19937& generator) const {
        std::normal_distribution<double> normal_dist(0.0, std::sqrt(sigma_beta_squared));
        std::vector<double> beta_samples(dimension);
        
        for (size_t i = 0; i < dimension; ++i) {
            beta_samples[i] = normal_dist(generator);
        }
        
        return beta_samples;
    }
    
    // Formula (16): σ²_β ~ Inv-Gamma(a_β, b_β)
    double generateInverseGammaPrior(double a_beta, double b_beta, std::mt19937& generator) const {
        // Generate using relationship: if X ~ Gamma(a, b), then 1/X ~ Inv-Gamma(a, b)
        std::gamma_distribution<double> gamma_dist(a_beta, 1.0 / b_beta);
        return 1.0 / gamma_dist(generator);
    }
    
    // Formula (17): τ ~ Beta(a_τ, b_τ)
    double generateBetaPrior(double a_tau, double b_tau, std::mt19937& generator) const {
        std::gamma_distribution<double> gamma1(a_tau, 1.0);
        std::gamma_distribution<double> gamma2(b_tau, 1.0);
        
        double x1 = gamma1(generator);
        double x2 = gamma2(generator);
        
        return x1 / (x1 + x2);
    }
    
    // Regime-switching expectile model
    // Formula (18): E_τ[r_t|S_t = j, F_{t-1}] = α_{τ,j} + Σ β_{τ,j,i}r_{t-i}
    double regimeSwitchingExpectile(const std::vector<double>& lagged_returns,
                                  int regime_state,
                                  const std::vector<std::vector<double>>& alpha_tau_regime,
                                  const std::vector<std::vector<std::vector<double>>>& beta_tau_regime) const {
        if (regime_state < 0 || regime_state >= static_cast<int>(alpha_tau_regime.size())) {
            throw std::invalid_argument("Invalid regime state");
        }
        
        double expectile_value = alpha_tau_regime[regime_state][0]; // α_{τ,j}
        
        // Add Σ β_{τ,j,i}r_{t-i} terms
        for (size_t i = 0; i < lagged_returns.size() && i < beta_tau_regime[regime_state].size(); ++i) {
            expectile_value += beta_tau_regime[regime_state][i][0] * lagged_returns[i];
        }
        
        return expectile_value;
    }
    
    // Markov chain transition probabilities
    // Formula (19): P(S_t = k|S_{t-1} = j) = p_{jk}
    std::vector<std::vector<double>> generateTransitionMatrix(int num_regimes, 
                                                            const std::vector<std::vector<double>>& transition_probs) const {
        if (transition_probs.size() != static_cast<size_t>(num_regimes)) {
            throw std::invalid_argument("Transition probability matrix dimension mismatch");
        }
        
        std::vector<std::vector<double>> transition_matrix(num_regimes, std::vector<double>(num_regimes));
        
        for (int j = 0; j < num_regimes; ++j) {
            if (transition_probs[j].size() != static_cast<size_t>(num_regimes)) {
                throw std::invalid_argument("Transition probability row dimension mismatch");
            }
            
            // Ensure row sums to 1
            double row_sum = std::accumulate(transition_probs[j].begin(), transition_probs[j].end(), 0.0);
            for (int k = 0; k < num_regimes; ++k) {
                transition_matrix[j][k] = transition_probs[j][k] / row_sum;
            }
        }
        
        return transition_matrix;
    }
    
    // Predict next regime state using transition probabilities
    int predictNextRegime(int current_regime, 
                         const std::vector<std::vector<double>>& transition_matrix,
                         std::mt19937& generator) const {
        if (current_regime < 0 || current_regime >= static_cast<int>(transition_matrix.size())) {
            throw std::invalid_argument("Invalid current regime state");
        }
        
        std::discrete_distribution<int> regime_dist(
            transition_matrix[current_regime].begin(), 
            transition_matrix[current_regime].end()
        );
        
        return regime_dist(generator);
    }
    
    // GARCH(1,1) volatility estimation
    // Formula (9): σ²_t = ω + αε²_{t-1} + βσ²_{t-1}
    std::vector<double> estimateGARCHVolatility(const std::vector<double>& residuals,
                                              double omega, double alpha, double beta) const {
        std::vector<double> variance(residuals.size());
        
        // Initial variance
        double mean_squared_residual = 0.0;
        for (double r : residuals) {
            mean_squared_residual += r * r;
        }
        mean_squared_residual /= residuals.size();
        variance[0] = mean_squared_residual;
        
        // GARCH iteration
        for (size_t t = 1; t < residuals.size(); ++t) {
            variance[t] = omega + alpha * residuals[t-1] * residuals[t-1] + 
                         beta * variance[t-1];
        }
        
        // Convert to volatility
        std::vector<double> volatility_series(variance.size());
        std::transform(variance.begin(), variance.end(), volatility_series.begin(),
                      [](double var) { return std::sqrt(var); });
        
        return volatility_series;
    }
    
    // Expectile-based Value-at-Risk (EVaR)
    double calculateEVaR(const std::vector<double>& data, double confidence_level) const {
        // Find corresponding tau that gives desired confidence level
        double tau = findTauForConfidenceLevel(data, confidence_level);
        return -calculateExpectile(data, tau); // Negative for loss convention
    }
    
    // Asymmetric Linear Loss function for backtesting
    // Formula (26): ALL_α = Σ(α - 1{r_t < VaR_{t,α}})(r_t - VaR_{t,α})
    double calculateAsymmetricLinearLoss(const std::vector<double>& actual_returns,
                                       const std::vector<double>& var_estimates,
                                       double alpha) const {
        if (actual_returns.size() != var_estimates.size()) {
            throw std::invalid_argument("Return and VaR vectors must have same size");
        }
        
        double total_loss = 0.0;
        
        for (size_t t = 0; t < actual_returns.size(); ++t) {
            double indicator = (actual_returns[t] < var_estimates[t]) ? 1.0 : 0.0;
            total_loss += (alpha - indicator) * (actual_returns[t] - var_estimates[t]);
        }
        
        return total_loss;
    }
    
    // Backtesting procedure - Unconditional Coverage test
    double unconditionalCoverageTest(const std::vector<double>& actual_returns,
                                   const std::vector<double>& var_estimates,
                                   double expected_violation_rate) const {
        int violations = 0;
        int total_observations = actual_returns.size();
        
        for (size_t t = 0; t < actual_returns.size(); ++t) {
            if (actual_returns[t] < var_estimates[t]) {
                violations++;
            }
        }
        
        double observed_rate = static_cast<double>(violations) / total_observations;
        
        // Likelihood ratio test statistic
        double lr_stat = -2.0 * std::log(
            std::pow(expected_violation_rate, violations) * 
            std::pow(1.0 - expected_violation_rate, total_observations - violations) /
            (std::pow(observed_rate, violations) * 
             std::pow(1.0 - observed_rate, total_observations - violations))
        );
        
        return lr_stat;
    }

private:
    // Helper function to find tau that corresponds to desired confidence level
    double findTauForConfidenceLevel(const std::vector<double>& data, double confidence_level) const {
        // Binary search for appropriate tau
        double tau_low = 0.001;
        double tau_high = 0.999;
        const double tolerance = 1e-6;
        
        while (tau_high - tau_low > tolerance) {
            double tau_mid = (tau_low + tau_high) / 2.0;
            double expectile_val = calculateExpectile(data, tau_mid);
            
            // Count how many observations are below this expectile
            int count_below = 0;
            for (double x : data) {
                if (x <= expectile_val) count_below++;
            }
            
            double empirical_prob = static_cast<double>(count_below) / data.size();
            
            if (empirical_prob < (1.0 - confidence_level)) {
                tau_low = tau_mid;
            } else {
                tau_high = tau_mid;
            }
        }
        
        return (tau_low + tau_high) / 2.0;
    }
};