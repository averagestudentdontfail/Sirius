#pragma once

#include <cmath>

// A simple dual number class for automatic differentiation.
// f(x + ε) = f(x) + f'(x)ε
// 'real' part stores f(x), 'dual' part stores the derivative f'(x)
template<typename T>
struct Dual {
    T real; // The value of the number
    T dual; // The derivative part

    Dual(T real_val = 0.0, T dual_val = 0.0) : real(real_val), dual(dual_val) {}

    // Operator overloads
    Dual<T> operator+(const Dual<T>& other) const {
        return Dual<T>(real + other.real, dual + other.dual);
    }
    Dual<T> operator-(const Dual<T>& other) const {
        return Dual<T>(real - other.real, dual - other.dual);
    }
    Dual<T> operator*(const Dual<T>& other) const {
        return Dual<T>(real * other.real, real * other.dual + dual * other.real);
    }
    Dual<T> operator/(const Dual<T>& other) const {
        T new_real = real / other.real;
        T new_dual = (dual * other.real - real * other.dual) / (other.real * other.real);
        return Dual<T>(new_real, new_dual);
    }
};

// --- Math functions for Dual numbers ---
template<typename T>
Dual<T> sin(const Dual<T>& d) {
    return Dual<T>(std::sin(d.real), d.dual * std::cos(d.real));
}

template<typename T>
Dual<T> cos(const Dual<T>& d) {
    return Dual<T>(std::cos(d.real), -d.dual * std::sin(d.real));
}

template<typename T>
Dual<T> sqrt(const Dual<T>& d) {
    T real_sqrt = std::sqrt(d.real);
    return Dual<T>(real_sqrt, d.dual / (2.0 * real_sqrt));
}

// Add other math functions (log, exp, etc.) as needed