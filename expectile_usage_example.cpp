#include "ebf.cpp"
#include <iomanip>
#include <fstream>
#include <sstream>

class FTSERiskAnalysis {
private:
    ExpectileRiskManager risk_manager;
    std::vector<double> ftse_returns;
    std::vector<double> volatility_series;
    std::vector<double> vix_data;
    
public:
    // Simulate realistic FTSE 100 return data based on paper statistics
    void generateFTSEData(int num_observations = 5217) {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Parameters from Table 1 in the paper
        double mean_return = 0.0002;     // 0.02% daily
        double std_deviation = 0.0121;   // 1.21% daily
        double skewness = -0.847;        // Negative skewness
        double excess_kurtosis = 9.564;  // Fat tails
        
        // Generate base normal returns
        std::normal_distribution<> normal_dist(mean_return, std_deviation);
        ftse_returns.resize(num_observations);
        volatility_series.resize(num_observations);
        vix_data.resize(num_observations);
        
        // Add regime-dependent characteristics
        std::uniform_real_distribution<> uniform(0.0, 1.0);
        
        for (int i = 0; i < num_observations; ++i) {
            double base_return = normal_dist(gen);
            
            // Add skewness and kurtosis effects based on paper characteristics
            double skew_adjustment = 0.0;
            if (uniform(gen) < 0.05) { // 5% extreme events reflecting excess kurtosis
                skew_adjustment = skewness * 0.01 * uniform(gen); // Apply negative skewness
            }
            
            ftse_returns[i] = base_return + skew_adjustment;
            
            // Generate corresponding volatility (GARCH-like)
            if (i == 0) {
                volatility_series[i] = std_deviation;
            } else {
                volatility_series[i] = 0.0001 + 0.08 * ftse_returns[i-1] * ftse_returns[i-1] + 
                                     0.9 * volatility_series[i-1] * volatility_series[i-1];
                volatility_series[i] = std::sqrt(volatility_series[i]);
            }
            
            // Generate VIX-like data (inversely correlated with returns)
            vix_data[i] = 20.0 + 15.0 * std::abs(ftse_returns[i]) / std_deviation + 
                         5.0 * normal_dist(gen);
        }
        
        // Set data in risk manager
        risk_manager.setReturns(ftse_returns);
        risk_manager.setVolatility(volatility_series);
        risk_manager.setVIXData(vix_data);
        
        std::cout << "Generated " << num_observations << " FTSE 100 observations\n";
        std::cout << "Target skewness: " << skewness << ", Target excess kurtosis: " << excess_kurtosis << "\n";
        std::cout << "Mean return: " << std::accumulate(ftse_returns.begin(), ftse_returns.end(), 0.0) / ftse_returns.size() << "\n";
        std::cout << "Volatility: " << calculateStandardDeviation(ftse_returns) << "\n\n";
    }
    
    // Example 1: Basic Expectile Analysis
    void demonstrateBasicExpectileAnalysis() {
        std::cout << "=== BASIC EXPECTILE ANALYSIS ===\n";
        
        // Calculate expectiles for different tau levels as in Table 2
        std::vector<double> tau_levels = {0.01, 0.05, 0.10, 0.90, 0.95, 0.99};
        
        std::cout << std::setw(8) << "Tau" << std::setw(15) << "Expectile" 
                  << std::setw(15) << "EVaR (%)" << "\n";
        std::cout << std::string(40, '-') << "\n";
        
        for (double tau : tau_levels) {
            double expectile = risk_manager.calculateExpectile(ftse_returns, tau);
            
            std::cout << std::setw(8) << std::fixed << std::setprecision(2) << tau
                      << std::setw(15) << std::setprecision(6) << expectile
                      << std::setw(15) << std::setprecision(2) << -expectile * 100 << "%\n";
        }
        std::cout << "\n";
    }
    
    // Example 2: Dynamic Expectile Levels (Formula 13)
    void demonstrateDynamicExpectileLevels() {
        std::cout << "=== DYNAMIC EXPECTILE LEVELS (Formula 13) ===\n";
        
        // Parameters similar to those that might be estimated for FTSE 100
        std::vector<double> delta_params = {0.1, 0.8, 2.5, 0.001};
        
        std::cout << "Time-varying expectile levels during different market conditions:\n";
        std::cout << std::setw(12) << "Period" << std::setw(15) << "Prev_Tau" 
                  << std::setw(15) << "Volatility" << std::setw(15) << "Return"
                  << std::setw(15) << "New_Tau" << "\n";
        std::cout << std::string(75, '-') << "\n";
        
        // Simulate different market conditions
        std::vector<std::string> periods = {"Calm", "Stress", "Crisis", "Recovery"};
        std::vector<double> prev_taus = {0.05, 0.03, 0.01, 0.02};
        std::vector<double> vols = {0.008, 0.025, 0.045, 0.018};
        std::vector<double> rets = {0.001, -0.015, -0.055, 0.008};
        
        for (size_t i = 0; i < periods.size(); ++i) {
            double new_tau = risk_manager.calculateDynamicExpectileLevel(
                prev_taus[i], vols[i], rets[i], delta_params);
            
            std::cout << std::setw(12) << periods[i]
                      << std::setw(15) << std::fixed << std::setprecision(3) << prev_taus[i]
                      << std::setw(15) << std::setprecision(3) << vols[i]
                      << std::setw(15) << std::setprecision(3) << rets[i]
                      << std::setw(15) << std::setprecision(3) << new_tau << "\n";
        }
        std::cout << "\n";
    }
    
    // Example 3: Multivariate Portfolio Analysis (Formula 14)
    void demonstrateMultivariatePortfolio() {
        std::cout << "=== MULTIVARIATE PORTFOLIO ANALYSIS (Formula 14) ===\n";
        
        // Simulate a 3-asset portfolio
        size_t num_assets = 3;
        
        // Create lagged returns matrix for each asset
        std::vector<std::vector<double>> lagged_returns(num_assets);
        std::vector<std::vector<double>> additional_vars(num_assets);
        
        for (size_t i = 0; i < num_assets; ++i) {
            lagged_returns[i] = {ftse_returns[100], ftse_returns[99]}; // r_{t-1}, r_{t-2}
            additional_vars[i] = {volatility_series[100], vix_data[100]}; // σ²_{t-1}, VIX_{t-1}
        }
        
        // Coefficient matrices (would be estimated from data)
        std::vector<double> alpha_tau = {-0.0198, -0.0201, -0.0195}; // From Table 2
        std::vector<std::vector<double>> B_matrices = {
            {-0.089, -0.032},  // Asset 1 AR coefficients
            {-0.085, -0.030},  // Asset 2 AR coefficients  
            {-0.092, -0.035}   // Asset 3 AR coefficients
        };
        std::vector<std::vector<double>> Gamma_matrices = {
            {1.923, 0.0008},   // Asset 1 volatility & VIX coefficients
            {1.890, 0.0007},   // Asset 2 volatility & VIX coefficients
            {1.950, 0.0009}    // Asset 3 volatility & VIX coefficients
        };
        
        auto portfolio_expectiles = risk_manager.multivariateExpectileFramework(
            lagged_returns, additional_vars, alpha_tau, B_matrices, Gamma_matrices);
        
        std::cout << "Portfolio expectile estimates:\n";
        for (size_t i = 0; i < num_assets; ++i) {
            std::cout << "Asset " << (i+1) << " Expectile: " 
                      << std::fixed << std::setprecision(6) << portfolio_expectiles[i] << "\n";
        }
        
        // Calculate portfolio-level risk
        std::vector<double> weights = {0.4, 0.35, 0.25};
        double portfolio_expectile = 0.0;
        for (size_t i = 0; i < num_assets; ++i) {
            portfolio_expectile += weights[i] * portfolio_expectiles[i];
        }
        std::cout << "Weighted Portfolio Expectile: " 
                  << std::fixed << std::setprecision(6) << portfolio_expectile << "\n\n";
    }
    
    // Example 4: Regime-Switching Model (Formula 18)
    void demonstrateRegimeSwitching() {
        std::cout << "=== REGIME-SWITCHING EXPECTILE MODEL (Formula 18) ===\n";
        
        // Define 3 market regimes: Calm, Stress, Crisis
        int num_regimes = 3;
        std::vector<std::string> regime_names = {"Calm", "Stress", "Crisis"};
        
        // Alpha parameters for each regime (would be estimated)
        std::vector<std::vector<double>> alpha_tau_regime = {
            {-0.0120},  // Calm regime
            {-0.0250},  // Stress regime  
            {-0.0420}   // Crisis regime
        };
        
        // Beta parameters for each regime and lag
        std::vector<std::vector<std::vector<double>>> beta_tau_regime(num_regimes);
        beta_tau_regime[0] = {{-0.065}, {-0.025}};  // Calm: weaker mean reversion
        beta_tau_regime[1] = {{-0.095}, {-0.035}};  // Stress: moderate mean reversion
        beta_tau_regime[2] = {{-0.145}, {-0.055}};  // Crisis: strong mean reversion
        
        // Transition probability matrix (Formula 19)
        std::vector<std::vector<double>> transition_probs = {
            {0.85, 0.12, 0.03},  // From Calm
            {0.25, 0.65, 0.10},  // From Stress
            {0.05, 0.35, 0.60}   // From Crisis
        };
        
        auto transition_matrix = risk_manager.generateTransitionMatrix(num_regimes, transition_probs);
        
        std::cout << "Regime transition probabilities:\n";
        std::cout << std::setw(10) << "From\\To" << std::setw(10) << "Calm" 
                  << std::setw(10) << "Stress" << std::setw(10) << "Crisis" << "\n";
        std::cout << std::string(40, '-') << "\n";
        
        for (int i = 0; i < num_regimes; ++i) {
            std::cout << std::setw(10) << regime_names[i];
            for (int j = 0; j < num_regimes; ++j) {
                std::cout << std::setw(10) << std::fixed << std::setprecision(2) 
                          << transition_matrix[i][j];
            }
            std::cout << "\n";
        }
        
        // Calculate expectiles for each regime
        std::vector<double> recent_returns = {ftse_returns[100], ftse_returns[99]};
        
        std::cout << "\nExpectile estimates by regime:\n";
        for (int regime = 0; regime < num_regimes; ++regime) {
            double regime_expectile = risk_manager.regimeSwitchingExpectile(
                recent_returns, regime, alpha_tau_regime, beta_tau_regime);
            
            std::cout << regime_names[regime] << " regime expectile: " 
                      << std::fixed << std::setprecision(6) << regime_expectile << "\n";
        }
        std::cout << "\n";
    }
    
    // Example 5: Bayesian Parameter Estimation (Formulas 15-17)
    void demonstrateBayesianEstimation() {
        std::cout << "=== BAYESIAN PARAMETER ESTIMATION (Formulas 15-17) ===\n";
        
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Prior parameters
        double sigma_beta_squared = 0.01;
        double a_beta = 2.0, b_beta = 0.5;
        double a_tau = 2.0, b_tau = 8.0;  // Favours lower tau values
        
        std::cout << "Drawing samples from Bayesian priors:\n\n";
        
        // Generate beta samples (Formula 15)
        auto beta_samples = risk_manager.generateBayesianBetaPrior(3, sigma_beta_squared, gen);
        std::cout << "Beta coefficient samples ~ N(0, " << sigma_beta_squared << "):\n";
        for (size_t i = 0; i < beta_samples.size(); ++i) {
            std::cout << "  β[" << i << "] = " << std::fixed << std::setprecision(4) 
                      << beta_samples[i] << "\n";
        }
        
        // Generate sigma_beta samples (Formula 16)
        std::cout << "\nVariance parameter samples ~ Inv-Gamma(" << a_beta << ", " << b_beta << "):\n";
        for (int i = 0; i < 5; ++i) {
            double sigma_sample = risk_manager.generateInverseGammaPrior(a_beta, b_beta, gen);
            std::cout << "  σ²_β[" << i << "] = " << std::fixed << std::setprecision(4) 
                      << sigma_sample << "\n";
        }
        
        // Generate tau samples (Formula 17)
        std::cout << "\nExpectile level samples ~ Beta(" << a_tau << ", " << b_tau << "):\n";
        for (int i = 0; i < 5; ++i) {
            double tau_sample = risk_manager.generateBetaPrior(a_tau, b_tau, gen);
            std::cout << "  τ[" << i << "] = " << std::fixed << std::setprecision(4) 
                      << tau_sample << "\n";
        }
        std::cout << "\n";
    }
    
    // Example 6: Risk Assessment and Backtesting
    void demonstrateRiskAssessmentBacktesting() {
        std::cout << "=== RISK ASSESSMENT AND BACKTESTING ===\n";
        
        // Split data for backtesting
        size_t split_point = ftse_returns.size() * 0.8;  // 80% training, 20% testing
        std::vector<double> training_data(ftse_returns.begin(), ftse_returns.begin() + split_point);
        std::vector<double> testing_data(ftse_returns.begin() + split_point, ftse_returns.end());
        
        // Calculate EVaR for different confidence levels
        std::vector<double> confidence_levels = {0.90, 0.95, 0.975, 0.99};
        
        std::cout << "EVaR estimates from training data:\n";
        std::cout << std::setw(15) << "Confidence" << std::setw(15) << "EVaR" 
                  << std::setw(15) << "EVaR (%)" << "\n";
        std::cout << std::string(45, '-') << "\n";
        
        std::vector<double> evar_estimates;
        for (double conf : confidence_levels) {
            double evar = risk_manager.calculateEVaR(training_data, conf);
            evar_estimates.push_back(evar);
            
            std::cout << std::setw(15) << std::fixed << std::setprecision(1) << conf * 100 << "%"
                      << std::setw(15) << std::setprecision(6) << evar
                      << std::setw(15) << std::setprecision(2) << evar * 100 << "%\n";
        }
        
        // Backtesting on 95% level
        double var_95 = evar_estimates[1];  // 95% EVaR
        std::vector<double> var_series(testing_data.size(), var_95);
        
        // Unconditional Coverage Test
        double uc_test = risk_manager.unconditionalCoverageTest(testing_data, var_series, 0.05);
        
        // Calculate violation rate
        int violations = 0;
        for (double ret : testing_data) {
            if (ret < var_95) violations++;
        }
        double violation_rate = static_cast<double>(violations) / testing_data.size();
        
        // Asymmetric Linear Loss
        double all_loss = risk_manager.calculateAsymmetricLinearLoss(testing_data, var_series, 0.05);
        
        std::cout << "\nBacktesting Results (95% EVaR):\n";
        std::cout << "Expected violations: 5.0%\n";
        std::cout << "Observed violations: " << std::fixed << std::setprecision(1) 
                  << violation_rate * 100 << "% (" << violations << "/" << testing_data.size() << ")\n";
        std::cout << "UC Test Statistic: " << std::setprecision(4) << uc_test << "\n";
        std::cout << "Asymmetric Linear Loss: " << std::setprecision(6) << all_loss << "\n";
        
        if (uc_test < 3.841) {  // Chi-square critical value at 5%
            std::cout << "Result: Model passes UC test (p > 0.05)\n";
        } else {
            std::cout << "Result: Model fails UC test (p < 0.05)\n";
        }
        std::cout << "\n";
    }
    
private:
    double calculateStandardDeviation(const std::vector<double>& data) {
        double mean = std::accumulate(data.begin(), data.end(), 0.0) / data.size();
        double sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), 0.0);
        return std::sqrt(sq_sum / data.size() - mean * mean);
    }
};

int main() {
    std::cout << "FTSE 100 EXPECTILE-BASED RISK MANAGEMENT FRAMEWORK\n";
    std::cout << "Based on: Quantitative Risk Management in Volatile Markets\n";
    std::cout << "Author: Abiodun F. Oketunji, University of Oxford\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    try {
        FTSERiskAnalysis analysis;
        
        // Generate realistic FTSE data
        analysis.generateFTSEData();
        
        // Run all demonstrations
        analysis.demonstrateBasicExpectileAnalysis();
        analysis.demonstrateDynamicExpectileLevels();
        analysis.demonstrateMultivariatePortfolio();
        analysis.demonstrateRegimeSwitching();
        analysis.demonstrateBayesianEstimation();
        analysis.demonstrateRiskAssessmentBacktesting();
        
        std::cout << "Analysis complete. All proposed mathematical formulas demonstrated.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}