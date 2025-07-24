## Expectile-Based Framework

[![Release](https://img.shields.io/github/release/0xnu/expectile-based-framework.svg)](https://github.com/0xnu/expectile-based-framework/releases/latest)
[![arXiv](https://img.shields.io/badge/arXiv-2507.13391-purple)](https://arxiv.org/abs/2507.13391)
[![License](https://img.shields.io/badge/License-Modified_MIT-f5de53?&color=f5de53)](/LICENSE)

This research presents a framework for quantitative risk management in volatile markets, specifically focusing on expectile-based methodologies applied to the FTSE 100 index. Traditional risk measures such as Value-at-Risk (VaR) have demonstrated significant limitations during periods of market stress, as evidenced during the 2008 financial crisis and subsequent volatile periods. This study develops an advanced expectile-based framework that addresses the shortcomings of conventional quantile-based approaches by providing greater sensitivity to tail losses and improved stability in extreme market conditions. The research employs a dataset spanning two decades of FTSE 100 returns, incorporating periods of high volatility, market crashes, and recovery phases. Our methodology introduces novel mathematical formulations for expectile regression models, enhanced threshold determination techniques using time series analysis, and robust backtesting procedures. The empirical results demonstrate that expectile-based Value-at-Risk (EVaR) consistently outperforms traditional VaR measures across various confidence levels and market conditions. The framework exhibits superior performance during volatile periods, with reduced model risk and enhanced predictive accuracy. Furthermore, the study establishes practical implementation guidelines for financial institutions and provides evidence-based recommendations for regulatory compliance and portfolio management. The findings contribute significantly to the literature on financial risk management and offer practical tools for practitioners dealing with volatile market environments.

### Building and Execution

The framework provides several build and execution options through its Makefile:

#### Basic Use

```bash
# Install dependencies (macOS)
make install-deps

# Build the framework
make

# Run the expectile-based risk analysis
make run

# Run standalone EBF library tests
make run-test
```

#### Advanced Options

```bash
# Compile with Clang++
make risk_analysis_clang

# Run performance benchmarks
make benchmark

# Validate model outputs against published results
make validate

# Compile with debug symbols
make debug

# Generate documentation from source comments
make docs
```

For a complete list of available targets:

```bash
make help
```

> **Note**: The framework supports both macOS (requires Homebrew) and Linux systems.

### License

This project is licensed under the [Modified MIT License](./LICENSE).

### Citation

```tex
@misc{expectilebasedframework,
  author       = {Oketunji, A.F.},
  title        = {Expectile-Based Risk Management Framework},
  year         = 2025,
  version      = {0.0.2},
  publisher    = {ArXiv},
  doi          = {arXiv:2507.13391},
  url          = {https://arxiv.org/abs/2507.13391}
}
```

### Copyright

(c) 2025 [Finbarrs Oketunji](https://finbarrs.eu). All Rights Reserved.
