#include <algorithm>
#include <cfloat>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <random>
#include <string>

#include "matrix.h"
#include "meshC.h"
#include "Mesh.h"

#define TOL 1e-12
#define MAXIT 1000000

#define SCHEME_UDS 0
#define SCHEME_CDS 1
#define SCHEME_EDS 2
#define SCHEME_HYBRID 3
#define SCHEME_POWERLAW 4

#define METHOD_GAUSS_SEIDEL 0
#define METHOD_LUP 1

// Computation of internal nodes discretization coefficients
double schemeUDS(const double P);
double schemeCDS(const double P);
double schemeHybrid(const double P);
double schemePowerlaw(const double P);
double schemeEDS(const double P);

void computeSteadyStateDiscretizationCoefficientsInternalNodes(const Mesh m, const double rho, const double gamma,
double (*vx)(double,double), double (*vy)(double, double), double (*sourceP)(double, double),
double (*sourceC)(double, double), const int schemeCode, double* A, double* b);

// Diagonal case functions
int solveDiagonalCase(const double L, const double lz, const int N, const double rho, const double gamma, const double phi_low, const double phi_high,
const int method, const bool print, const bool plot, const char* filename);

void computeDiscCoefsBoundaryNodesDiagonalCase(const Mesh m, const double phi_low, const double phi_high, double* A, double* b);
double vxDiagonal(const double, const double);
double vyDiagonal(const double, const double);
double sourcePDiagonal(const double, const double);
double sourceCDiagonal(const double, const double);

// Smith-Hutton case functions
int solveSmithHuttonCase(const double L, const double lz, const int N, const double rho, const double gamma,
const int method, const bool print, const bool plot, const char* filename);
void computeDiscretizationCoefficientsBoundaryNodesSmithHuttonCase(const Mesh m, double* A, double* b);
double vxSmithHutton(const double x, const double y);
double vySmithHutton(const double x, const double y);
double sourcePSmithHutton(const double, const double);
double sourceCSmithHutton(const double, const double);

// Pre-solving check functions
void checkSystemMatrix(const int nx, const int ny, const double tol, const double* A);

// Linear system solve functions
int solveSystem(const int nx, const int ny, const double* A, const double* b, double* phi, const int method);
int solveSystemGS(const int nx, const int ny, const double tol, const int maxIt, const double* A, const double* b, double* phi);
int solveSystemLUP(const int nx, const int ny, const double* A, const double* b, double* phi);
void assembleMatrix(const int nx, const int ny, const double* A, double** AA);
void assembleMatrix(const int nx, const int ny, const double* A, double* AA);

// Print results functions
void printToFile(const Mesh m, const double* phi, const char* filename, const int precision);
void plotSolution(const Mesh m, const char* filename);
void printVelocityField(const Mesh m, double (*vx)(double, double), double (*vy)(double, double), const char* filename_mod, const char* filename_vec, const int precision);

// Check solution functions
double checkLinearSystemSolution(const int nx, const int ny, const double* A, const double* b, const double* phi);

int main(int argc, char* argv[]) {

    // Physical data
    double L = 1;   // Characteristic length    [m]
    double lz = 1;  // Domain size in z axis    [m]

    // Numerical data
    int N = 200;    // Nodes for uniform discretization

    // Thermophysical properties
    const double rho = 1000;        // Density                  [kg/m^3]
    const double gamma = rho/1e3;   // Diffusion coefficient

    const char* filename = "test.dat";
    const double phi_low = 0;
    const double phi_high = 1;
    solveDiagonalCase(L, lz, N, rho, gamma, phi_low, phi_high, METHOD_GAUSS_SEIDEL, true, true, filename);

    // solveSmithHuttonCase(L, lz, N, rho, gamma, METHOD_GAUSS_SEIDEL, true, true, filename);

    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// COMPUTATION OF INTERNAL NODES DISCRETIZATION COEFFICIENTS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
double schemeUDS(const double P) {
    /*
    schemeUDS: computes the contribution to the discretization coefficient according to the Upwind-Difference Scheme (UDS)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - P         P??clet's number     [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - double    Contribution to the discretization coefficient
    */
    return 1;
}

double schemeCDS(const double P) {
    /*
    schemeCDS: computes the contribution to the discretization coefficient according to the Central-Difference Scheme (CDS)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - P         P??clet's number     [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - double    Contribution to the discretization coefficient
    */
    return 1 - 0.5 * std::abs(P);
}

double schemeHybrid(const double P) {
    /*
    schemeHybrid: computes the contribution to the discretization coefficient according to the Hybrid scheme
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - P         P??clet's number     [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - double    Contribution to the discretization coefficient
    */
    return std::max(0.0, 1 - 0.5 * std::abs(P));
}

double schemePowerlaw(const double P) {
    /*
    schemePowerlaw: computes the contribution to the discretization coefficient according to the Powerlaw scheme
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - P         P??clet's number     [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - double    Contribution to the discretization coefficient
    */
    return std::max(0.0, std::pow(1 - 0.1*std::abs(P), 5));
}

double schemeEDS(const double P) {
    /*
    schemeEDS: computes the contribution to the discretization coefficient according to the Exponential-Difference Scheme (EDS)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - P         P??clet's number     [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - double    Contribution to the discretization coefficient
    */
    return std::abs(P)/(std::exp(std::abs(P)) - 1);
}

void computeSteadyStateDiscretizationCoefficientsInternalNodes(const Mesh m, const double rho, const double gamma,
double (*vx)(double,double), double (*vy)(double, double), double (*sourceP)(double, double),
double (*sourceC)(double, double), const int schemeCode, double* A, double* b) {
    /*
    computeSteadyStateDiscretizationCoefficientsInternalNodes: computes the discretization coefficients for the internal nodes in a steady state
    convection diffusion problem in a 2D cartesian mesh.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - m             Mesh object                                                                     [const Mesh]
        - rho           Density                                                                         [const double]
        - gamma         Diffusion coefficient                                                           [const double]
        - *vx           Function that gives the velocity in the x axis at (x,y)                         [returns double, needs (double,double)]
        - *vy           Function that gives the velocity in the y axis at (x,y)                         [returns double, needs (double,double)]
        - *sourceP      Function that gives the P source term at (x,y)                                  [returns double, needs (double,double)]
        - *sourceC      Function that gives the C source term at (x,y)                                  [returns double, needs (double,double)]
        - schemeCode    Integer that tells what scheme is used to compute the convective properties     [const int]
        - A             Linear system matrix set to zero. Rows: nx*ny, Columns: 5                       [double*]
        - b             Vector of indenpendent terms set to zero. Rows: nx*ny, Columns: 1               [double*]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - A         Linear system matrix. Rows: nx*ny, Columns: 5                            [double*]
        - b         Vector of indenpendent terms. Rows: nx*ny, Columns: 1                    [double*]
    */
    // Choose scheme to compute the convective properties
    double (*scheme) (const double);
    switch(schemeCode) {
        // UDS scheme
        case SCHEME_UDS:
            scheme = &schemeUDS;
            break;
        // CDS scheme
        case SCHEME_CDS:
            scheme = &schemeCDS;
            break;
        // EDS scheme
        case SCHEME_EDS:
            scheme = &schemeEDS;
            break;
        // Hybrid scheme
        case SCHEME_HYBRID:
            scheme = &schemeHybrid;
            break;
        // Powerlaw scheme
        default:    // Powerlaw scheme
            scheme = &schemePowerlaw;
    }

    // Initialize matrix of discretization coefficients (A) and vector of independent terms (b) to zero
    std::fill_n(A, 5*m.getNX()*m.getNY(), 0);
    std::fill_n(b, m.getNX()*m.getNY(), 0);

    // Internal nodes
    for(int j = 1; j < m.getNY()-1; j++) {
        for(int i = 1; i < m.getNX()-1; i++) {
            int node = j * m.getNX() + i;
            double x = m.atNodeX(i);
            double y = m.atNodeY(j);
            // South node
            double D = gamma * m.atSurfY(i) / m.atDistY(j-1);
            double F = rho * (*vy)(x, m.atFaceY(j)) * m.atSurfY(i);
            double P = F / D;
            A[5*node] = D * (*scheme)(P) + std::max(F, 0.0);
            // West node
            D = gamma * m.atSurfX(j) / m.atDistX(i-1);
            F = rho * (*vx)(m.atFaceX(i), y) * m.atSurfX(j);
            P = F / D;
            A[5*node+1] = D * (*scheme)(P) + std::max(F, 0.0);
            // East node
            D = gamma * m.atSurfX(j) / m.atDistX(i);
            F = rho * (*vx)(m.atFaceX(i+1), y) * m.atSurfX(j);
            P = F / D;
            A[5*node+2] = D * (*scheme)(P) + std::max(-F, 0.0);
            // North node
            D = gamma * m.atSurfY(i) / m.atDistY(j);
            F = rho * (*vy)(x, m.atFaceY(j+1)) * m.atSurfY(i);
            P = F / D;
            A[5*node+3] = D * (*scheme)(P) + std::max(-F, 0.0);
            // Central node
            A[5*node+4] = A[5*node] + A[5*node+1] + A[5*node+2] + A[5*node+3] - (*sourceP)(x,y) * m.atVol(i,j);
            // Independent term
            b[node] = (*sourceC)(x,y) * m.atVol(i,j);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DIAGONAL CASE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int solveDiagonalCase(const double L, const double lz, const int N, const double rho, const double gamma, const double phi_low, const double phi_high,
const int method, const bool print, const bool plot, const char* filename) {
    /*
    solveDiagonalCase: solves the diagonal case problem given the geometry, the thermophysical properties and the boundary conditions.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - l             Characteristic length               [const double]
        - lz            Domain size in z axis               [const double]
        - N             Nodes for uniform discretization    [const int]
        - rho           Density                             [const double]
        - gamma         Diffusion coefficient               [const double]
        - phi_low       Boundary condition                  [const double]
        - phi_high      Boundary condition                  [const double]
        - method        Method to solve the linear system   [const int]
        - print         True: the solution is printed to the file named filename. False: the solution is not printed                                                        [const bool]
        - plot          True: the solution is ploted with the data in the file named filename. False: the solution is not plotted. In order to work need print to be true.  [const bool]
        - filename      Name of the file where the solution is printed and/or plotted
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - int           Exit code. 1: Everything nominal. -1: Could not build mesh. -2: Could not allocate memory with malloc at some point.
    */

    // MESH BUILDING
    printf("Building uniform cartesian mesh...\n\n");
    Mesh m;
    m.buildUniformMesh(0, 0, L, L, lz, N, N);
    if(!m.isBuilt()) {
        printf("\tError. Could not build mesh\n");
        return -1;
    }

    // DISCRETIZATION COEFFICIENTS COMPUTATION
    printf("Computing discretization coefficients for the diagonal case...\n");
    // Allocate memory
    printf("\tAllocating memory for the linear system matrix...\n");
    double* A = (double*) malloc(5 * m.getNX() * m.getNY() * sizeof(double*));
    if(!A) {
        printf("\tError. Could not allocate memory for the linear system matrix.\n");
        return -2;
    }

    printf("\tAllocating memory for the linear system vector...\n");
    double* b = (double*) malloc(m.getNX() * m.getNY() * sizeof(double*));
    if(!b) {
        free(A);
        printf("\tError. Could not allocate memory for the linear system vector.\n");
        return -2;
    }

    printf("\tComputing internal nodes discretization coefficients...\n");
    computeSteadyStateDiscretizationCoefficientsInternalNodes(m, rho, gamma, vxDiagonal, vyDiagonal, sourcePDiagonal, sourceCDiagonal, SCHEME_UDS, A, b);

    printf("\tComputing boundary nodes discretization coefficients...\n\n");
    computeDiscCoefsBoundaryNodesDiagonalCase(m, phi_low, phi_high, A, b);

    // LINEAR SYSTEM SOLVING
    // Allocate memory for the linear system solution vector
    printf("Solving the linear system...\n");
    printf("\tAllocating memory for the linear system solution...\n");
    double* phi = (double*) malloc(m.getNX() * m.getNY() * sizeof(double*));
    if(!phi) {
        free(A);
        free(b);
        printf("\tError. Could not allocate memory for the linear system solution vector.\n");
        return -2;
    }
    std::fill_n(phi, m.getNX()*m.getNY(), 1);

    // Solve the linear system
    int exitSolve = solveSystem(m.getNX(), m.getNY(), A, b, phi, method);

    // Check linear system solution, set exitCode and print/plot results
    int exitCode = 0;
    if(exitSolve > 0) {
        exitCode = 1;
        double error = checkLinearSystemSolution(m.getNX(), m.getNY(), A, b, phi);
        printf("\tChecking the solution... Infinity norm of A*x-b: %.2e\n\n", error);
        // PRINT AND/OR PLOT SOLUTION
        if(print) {
            printToFile(m, phi, filename, 5);
            if(plot)
                plotSolution(m, filename);
        }
    } else
        exitCode = -3;

    // Free memory allocated
    printf("Freeing memory allocated...\n");
    free(A);
    free(b);
    free(phi);
    return exitCode;
}

void computeDiscCoefsBoundaryNodesDiagonalCase(const Mesh m, const double phi_low, const double phi_high, double* A, double* b) {
    /*
    computeDiscCoefsBoundaryNodesDiagonalCase: computes the discretization coefficients for the boundary nodes in the diagonal case
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - m                 Mesh object                                                                     [const Mesh]
        - phi_low           Minimum value of phi on the boundary                                            [const double]
        - phi_high          Maximum value of phi on the boundary                                            [const double]
        - A                 Linear system matrix set to zero. Rows: nx*ny, Columns: 5                       [double*]
        - b                 Vector of indenpendent terms set to zero. Rows: nx*ny, Columns: 1               [double*]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - A                 Linear system matrix. Rows: nx*ny, Columns: 5                                   [double*]
        - b                 Vector of indenpendent terms. Rows: nx*ny, Columns: 1                           [double*]
    */
    // printf("Computing boundary nodes discretization coefficients for the diagonal case...\n\n");
    // Lower row nodes: 0 <= i <= nx-1; j=0
    for(int i = 0; i < m.getNX(); i++) {
        A[5*i+4] = 1;
        b[i] = phi_low;
    }
    // Right column nodes: i = nx-1; 1 <= j <= ny-1
    for(int j = 1; j < m.getNY(); j++) {
        int node = (j + 1) * m.getNX() - 1;
        A[5*node+4] = 1;
        b[node] = phi_low;
    }
    // Left column nodes: i = 0; 1 <= j <= ny-1
    for(int j = 1; j < m.getNY(); j++) {
        int node = j * m.getNX();
        A[5*node+4] = 1;
        b[node] = phi_high;
    }
    // Upper row nodes: 1 <= i <= nx-2, j = ny-1
    for(int i = 1; i < m.getNX()-1; i++) {
        int node = (m.getNY() - 1) * m.getNX() + i;
        A[5*node+4] = 1;
        b[node] = phi_high;
    }
}

double vxDiagonal(const double x, const double y) {
    /*
    vxDiagonal: computes the x component of velocity for the diagonal case at the point (x,y)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - x     x coordinate    [const double]
        - y     y coordinate    [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - vx    x component of the velocity for the diagonal case at point (x,y)    [double]
    */
    return 1*cos(0.25*M_PI);
}

double vyDiagonal(const double x, const double y) {
    /*
    vyDiagonal: computes the y component of velocity for the diagonal case at the point (x,y)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - x     x coordinate    [const double]
        - y     y coordinate    [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - vy    y component of the velocity for the diagonal case at point (x,y)    [double]
    */
    return 1*sin(0.25*M_PI);
}

double sourcePDiagonal(const double x, const double y) {
    /*
    sourcePDiagonal: computes the SP coefficient of the source term for the diagonal case at the point (x,y)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - x         x coordinate    [const double]
        - y         y coordinate    [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - source    SP coefficient of the source term for the diagonal case at the point (x,y)  [double]
    */
    return 0;
}

double sourceCDiagonal(const double x, const double y) {
    /*
    sourcePDiagonal: computes the SC coefficient of the source term for the diagonal case at the point (x,y)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - x         x coordinate    [const double]
        - y         y coordinate    [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - source    SC coefficient of the source term for the diagonal case at the point (x,y)  [double]
    */
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SMITH-HUTTON CASE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int solveSmithHuttonCase(const double L, const double lz, const int N, const double rho, const double gamma,
const int method, const bool print, const bool plot, const char* filename) {
    /*
    solveSmithHuttonCase: solves the Smith-Hutton case given the geometry, the thermophysical properties and the boundary conditions.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - l             Characteristic length               [const double]
        - lz            Domain size in z axis               [const double]
        - N             Nodes for uniform discretization    [const int]
        - rho           Density                             [const double]
        - gamma         Diffusion coefficient               [const double]
        - method        Method to solve the linear system   [const int]
        - print         True: the solution is printed to the file named filename. False: the solution is not printed                                                        [const bool]
        - plot          True: the solution is ploted with the data in the file named filename. False: the solution is not plotted. In order to work need print to be true.  [const bool]
        - filename      Name of the file where the solution is printed and/or plotted
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - int           Exit code. 1: Everything nominal. -1: Could not build mesh. -2: Could not allocate memory with malloc at some point.
                        -3: an error ocurred while the linear system was being solved.
    */

    // Build mesh
    const int nx = (N % 2 == 1 ? N : N+1);  // Number of nodes along x axis. Has to be an odd number
    const int ny = 0.5*(nx + 1);            // Number of nodes along y axis for uniform discretization.

    printf("Building uniform cartesian mesh...\n\n");
    Mesh m;
    m.buildUniformMesh(-L, 0, 2*L, L, lz, nx, ny);
    if(!m.isBuilt()) {
        printf("\tError. Could not build mesh\n");
        return -1;
    }

    // DISCRETIZATION COEFFICIENTS COMPUTATION
    printf("Computing discretization coefficients for the Smith-Hutton case...\n");
    // Allocate memory
    printf("\tAllocating memory for the linear system matrix...\n");
    double* A = (double*) malloc(5 * m.getNX() * m.getNY() * sizeof(double*));
    if(!A) {
        printf("\tError. Could not allocate memory for the linear system matrix.\n");
        return -2;
    }

    printf("\tAllocating memory for the linear system vector...\n");
    double* b = (double*) malloc(m.getNX() * m.getNY() * sizeof(double*));
    if(!b) {
        free(A);
        printf("\tError. Could not allocate memory for the linear system vector.\n");
        return -2;
    }

    // Compute discretization coefficients
    printf("\tComputing internal nodes discretization coefficients...\n");
    computeSteadyStateDiscretizationCoefficientsInternalNodes(m, rho, gamma, vxSmithHutton, vySmithHutton, sourcePSmithHutton, sourceCSmithHutton, SCHEME_POWERLAW, A, b);

    printf("\tComputing boundary nodes dicretization coefficients...\n\n");
    computeDiscretizationCoefficientsBoundaryNodesSmithHuttonCase(m, A, b);

    // LINEAR SYSTEM SOLVING
    // Allocate memory for the linear system solution vector
    printf("Solving the linear system...\n");
    printf("\tAllocating memory for the linear system solution...\n");
    double* phi = (double*) malloc(m.getNX() * m.getNY() * sizeof(double*));
    if(!phi) {
        free(A);
        free(b);
        printf("\tError. Could not allocate memory for the linear system solution vector.\n");
        return -2;
    }
    std::fill_n(phi, m.getNX()*m.getNY(), 1);

    // Solve the linear system
    int exitSolve = solveSystem(m.getNX(), m.getNY(), A, b, phi, method);

    // Check linear system solution, set exitCode and print/plot results
    int exitCode = 0;
    if(exitSolve > 0) {
        exitCode = 1;
        double error = checkLinearSystemSolution(m.getNX(), m.getNY(), A, b, phi);
        printf("\tChecking the solution... Infinity norm of A*x-b: %.2e\n\n", error);
        // PRINT AND/OR PLOT SOLUTION
        if(print) {
            printToFile(m, phi, filename, 5);
            if(plot)
                plotSolution(m, filename);
        }
    } else
        exitCode = -3;

    // Free memory allocated
    printf("Freeing memory allocated...\n");
    free(A);
    free(b);
    free(phi);
    return exitCode;
}

void computeDiscretizationCoefficientsBoundaryNodesSmithHuttonCase(const Mesh m, double* A, double* b) {
    /*
    computeDiscretizationCoefficientsBoundaryNodesSmithHuttonCase: computes the boundary nodes discretization coefficient for the Smith-Hutton case
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - m     Mesh object                                                                     [const Mesh]
        - A     Linear system matrix set to zero. Rows: nx*ny, Columns: 5                       [double*]
        - b     Vector of indenpendent terms set to zero. Rows: nx*ny, Columns: 1               [double*]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - A     Linear system matrix set to zero. Rows: nx*ny, Columns: 5                       [double*]
        - b     Vector of indenpendent terms set to zero. Rows: nx*ny, Columns: 1               [double*]
    */
    // Lower boundary nodes: 0 <= i <= nx-1; j=0
    // Lower boundary: [-L, 0]
    for(int i = 0; i < m.getNX() && m.atNodeX(i) <= 0; i++) {
        A[5*i+4] = 1;
        b[i] = 1 + tanh(10*(2*m.atNodeX(i)+1));
    }

    // Lower boundary: (0,L]
    for(int i = m.getNX()-1; i >= 0 && m.atNodeX(i) > 0; --i) {
        A[5*i+3] = 1;
        A[5*i+4] = 1;
    }

    // Left and right boundaries
    for(int j = 1; j < m.getNY(); j++) {
        // Left boundary
        int node = j*m.getNX();
        A[5*node+4] = 1;
        b[node] = 1 - tanh(10);
        // Right boundary
        node = (j+1)*m.getNX() - 1;
        A[5*node+4] = 1;
        b[node] = 1 - tanh(10);
    }

    // Top boundary
    for(int i = 1; i < m.getNX()-1; i++) {
        int node = (m.getNY()-1)*m.getNX() + i;
        A[5*node+4] = 1;
        b[node] = 1 - tanh(10);
    }
}

double vxSmithHutton(const double x, const double y) {
    /*
    vxSmithHutton: computes the x component of velocity for the Smith-Hutton case at the point (x,y)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - x         x coordinate    [const double]
        - y         y coordinate    [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - vx        x component of the velocity for the Smith-Hutton case at point (x,y)    [double]
    */
    return 2*y*(1 - x*x);
}

double vySmithHutton(const double x, const double y) {
    /*
    vxSmithHutton: computes the y component of velocity for the Smith-Hutton case at the point (x,y)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - x         x coordinate    [const double]
        - y         y coordinate    [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - vy        y component of the velocity for the Smith-Hutton case at point (x,y)    [double]
    */
    return -2*x*(1 - y*y);
}

double sourcePSmithHutton(const double, const double) {
    /*
    sourcePSmithHutton: computes the SP coefficient of the source term for the Smith-Hutton case at the point (x,y)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - x         x coordinate    [const double]
        - y         y coordinate    [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - source    SP coefficient of the source term for the Smith-Hutton case at the point (x,y)  [double]
    */
    return 0;
}

double sourceCSmithHutton(const double, const double) {
    /*
    sourceCSmithHutton: computes the SC coefficient of the source term for the Smith-Hutton case at the point (x,y)
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - x         x coordinate    [const double]
        - y         y coordinate    [const double]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - source    SC coefficient of the source term for the Smith-Hutton case at the point (x,y)  [double]
    */
    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PRE-SOLVING CHECK FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void checkSystemMatrix(const int nx, const int ny, const double tol, const double* A) {
    /*
    checkSystemMatrix: checks
        - whether the system matrix has a row full of zeros (sufficient condition for incompatible system but not necessary)
        - whether some coefficient aP (5th column) has a zero (prevent division by 0)
    In either case the function prints a message.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - nx: discretization nodes in X axis    [const int]
        - ny: discretization nodes in Y axis    [const int]
        - tol: tolerance                        [const double]
        - A: linear system matrix               [double*]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs: none
    */
    printf("Checking linear system matrix...\n");
    // Check whether the system matrix has a row full of zeros (sufficient condition for incompatible system but not necessary)
    bool foundNullRow = false;  // Bool variable: true => a null row has been found; false => no null row has been found
    int node = 0;      // Node (row) being checked
    while(node < nx*ny && !foundNullRow) {
        bool foundNonZeroElement = false;   // Bool variable: true => an element different from zero has been found in the row; false => no element different from zero has been found in the row
        int col = 0;                        // Column that is being checked
        // Check columns
        while(col < 5 && !foundNonZeroElement) {
            if(std::abs(A[5*node+col]) > tol)
                foundNonZeroElement = true;
            col++;
        }
        if(!foundNonZeroElement) {
            foundNullRow = true;
            printf("Found null row: %d\n", node);
        }
        node++;
    }

    // Check whether some coefficient aP (5th column) has a zero (prevent division by 0)
    bool nullCoef = false;  // True => a null aP coefficient has been found; false => no null aP coefficient has been foudn
    node = 0;               // Node (row) being checked
    while(node < nx*ny && !nullCoef) {
        nullCoef = (std::abs(A[5*node+4]) < tol);
        node++;
    }
    if(nullCoef) {
        printf("Found null aP coefficient, row: %d\n", node-1);
    }
    printf("\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// lINEAR SYSTEM SOLVING FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int solveSystem(const int nx, const int ny, const double* A, const double* b, double* phi, const int method) {
    /*
    solveSystem: solves the linear system resulting from a 2D convection-diffusion problem in a domain discretized with a cartesian mesh using the
    specified method.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - nx        Number of nodes in the X axis                                       [const int]
        - ny        Number of nodes in the Y axis                                       [const int]
        - A         Linear system matrix. Rows: nx*ny, Columns: 5                       [double*]
        - b         Vector of indenpendent terms. Rows: nx*ny, Columns: 1               [double*]
        - phi       Solution of the linar system. Rows: nx*ny, Columns: 1               [double*]
        - metho     Method to solve the linear system. 0 - Gauss-Seidel, Other - LUP    [int]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - phi       Solution of the linar system. Rows: nx*ny, Columns: 1               [double*]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Return:
        - int       Depends on the method.
                        - For Gauss-Seidel method, when there is convergence it returns the number of iterations. Otherwise it returns -1.
                        - For LUP method
    */
    // printf("Solving linear system using Gauss-Seidel method...\n");
    // printf("Solving linear system using LUP factorization...\n");

    // Solve the linear system
    int exitSolve = 0;
    if(method == METHOD_LUP) {
        // Solve using LUP decomposition
        printf("\tSolving the linear system using LUP decomposition...\n");
        exitSolve = solveSystemLUP(nx, ny, A, b, phi);
        if(exitSolve > 0)
            printf("\tLUP decomposition exited successfully.\n");
        else if(exitSolve == -1)
            printf("\tError. An error occurred during LUP decomposition. Linear system matrix is singular or close to singular.\n\n");
        else
            printf("\tError. An error occurred during LUP decomposition. Could not allocate enough memory.\n\n");
    } else {
        // Solve using Gauss--Seidel algorithm
        printf("\tSolving the linear system using Gauss-Seidel algorithm...\n");
        exitSolve = solveSystemGS(nx, ny, TOL, MAXIT, A, b, phi);
        if(exitSolve > 0)
            printf("\tGauss-Seidel algorithm exited successfully. Iterations: %d\n", exitSolve);
        else
            printf("\tError. No convergence of solution.\n\n");
    }
    return exitSolve;
}

int solveSystemGS(const int nx, const int ny, const double tol, const int maxIt, const double* A, const double* b, double* phi) {
    /*
    solveSystemGS: solves the linear system resulting from a 2D convection-diffusion problem in a domain discretized with a cartesian mesh using
    Gauss-Seidel algorithm. It has two criterion to stop the iteration:
        - Let phi* and phi be two consecutive vectors of the sequence produced by Gauss-Seidel algorithm. If the infinity norm of phi-phi* is less
        than tol, then the algorithm stops.
        - If the number of iterations reach maxIt, the algorithm stops.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - nx        Number of nodes in the X axis                               [const int]
        - ny        Number of nodes in the Y axis                               [const int]
        - tol       Tolerance to stop iteration                                 [const double]
        - maxIt     Maximum number of iterations                                [const int]
        - A         Linear system matrix. Rows: nx*ny, Columns: 5               [double*]
        - b         Vector of indenpendent terms. Rows: nx*ny, Columns: 1       [double*]
        - phi       Solution of the linar system. Rows: nx*ny, Columns: 1       [double*]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - phi       Solution of the linar system. Rows: nx*ny, Columns: 1       [double*]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Return:
        - int       If the algorithm reaches the max number of iterations with no convergence, it returns -1. Otherwise returns the number of iterations.
    */
    // printf("Solving linear system...\n");
    int it = 0;        // Current iteration
    bool convergence = false;   // Boolean variable to tell whether there is convergence or not. False: no convergence, True: convergence
    // Gauss-Seidel iteration
    while(it < maxIt && !convergence) {
        double maxDiff = -1;    // Infinity norm of the difference phi-phi*
        // Lower row nodes
        for(int i = 0; i < nx; i++) {
            int node = i;                                                       // Node whose phi is being computed
            double aux = phi[node];                                             // Previous value of phi[node]
            phi[node] = (b[node] + A[5*node+3] * phi[node+nx]) / A[5*node+4];   // Compute new value
            maxDiff = std::max(maxDiff, std::abs(aux - phi[node]));             // Update infinity norm
        }
        // Central rows nodes
        for(int j = 1; j < ny-1; j++) {
            for(int i = 0; i < nx; i++) {
                int node = j * nx + i;                                          // Node whose phi is being computed
                double aux = phi[node];                                         // Previous value of phi[node]
                phi[node] = (b[node] + A[5*node] * phi[node-nx] + A[5*node+1] * phi[node-1] + A[5*node+2] * phi[node+1] + A[5*node+3] * phi[node+nx]) / A[5*node+4];    // Compute new value
                maxDiff = std::max(maxDiff, std::abs(aux - phi[node]));         // Update infinity norm
            }
        }
        // Upper row nodes
        for(int i = 0; i < nx; i++) {
            int node = (ny - 1) * nx + i;                                       // Node whose phi is being computed
            double aux = phi[node];                                             // Previous value of phi[node]
            phi[node] = (b[node] + A[5*node] * phi[node-nx]) / A[5*node+4];     // Compute new value
            maxDiff = std::max(maxDiff, std::abs(aux - phi[node]));             // Update infinity norm
        }
        // Final checks of the current iteration
        convergence = (maxDiff < tol);  // Convergence condition
        it++;                           // Increase iteration counter
    }
    // Return value
    if(convergence)
        return it;
    return -1;
}

int solveSystemLUP(const int nx, const int ny, const double* A, const double* b, double* phi) {
    /*
    solveSystemLUP: solves the linear system resulting from a 2D convection-diffusion problem in a domain discretized with a cartesian mesh using
    the LUP factorization.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - nx        Number of nodes in the X axis                               [const int]
        - ny        Number of nodes in the Y axis                               [const int]
        - A         Linear system matrix. Rows: nx*ny, Columns: 5               [double*]
        - b         Vector of indenpendent terms. Rows: nx*ny, Columns: 1       [double*]
        - phi       Solution of the linar system. Rows: nx*ny, Columns: 1       [double*]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - phi       Solution of the linar system. Rows: nx*ny, Columns: 1       [double*]
    */

    int exit = 0;       // Exit code
    int n = nx * ny;    // Number of nodes (size of the linear system matrix)
    double* AA = (double*) malloc(n * n * sizeof(double*)); // Linear system matrix
    // Check if memory was allocated
    if(AA) {
        // Enough memory, continue solving
        std::fill_n(AA, n*n, 0);
        assembleMatrix(nx, ny, A, AA);
        // LUP method
        int* perm = (int*) malloc(n * sizeof(int*));
        int permSign = factorLU(AA, perm, n, TOL);
        // Check the permutation sign
        if(permSign == 0) {
            // Matrix is singular, error
            std::fill_n(phi, n, 0);
            exit = -1;
        } else {
            // Solve the triangular linear systems
            solveLUP(AA, b, phi, perm, n);
            exit = 1;
        }
        free(AA);
    } else {
        // Not enough memory, error
        std::fill_n(phi, n, 0);
        exit = -2;
    }
    return exit;
}

void assembleMatrix(const int nx, const int ny, const double* A, double** AA) {
    /*
    assembleMatrix: given the matrix A (is in vector form), the function assembles the matrix AA (square matrix) for LUP method. It is assumed that
    AA is already allocated.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - nx        Number of nodes in the X axis                               [const int]
        - ny        Number of nodes in the Y axis                               [const int]
        - A         Linear system matrix. Rows: nx*ny, Columns: 5               [const double*]
        - AA        Matrix to be assembled (in square matrix form)              [double**]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs: temperature map of phi. Prior to running the program,
        - AA        Assembled matrix (in square matrix form)                    [double**]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    */
    // // Lower row
    for(int i = 0; i < nx; i++) {
        int node = i;
        AA[node][node+nx] = -A[5*node+3]; // aN coefficient
        AA[node][node] = A[5*node+4];    // aP coefficient
    }
    // // Central rows
    for(int j = 1; j < ny-1; j++) {
        // First node
        int node = j * nx;
        AA[node][node+1] = -A[5*node+2];  // aE coefficient
        AA[node][node] = A[5*node+4];    // aP coefficient
        // Central nodes
        for(int i = 1; i < nx-1; i++) {
            node = j * nx + i;
            AA[node][node-nx] = -A[5*node];   // aS coefficient
            AA[node][node-1]  = -A[5*node+1]; // aW coefficient
            AA[node][node+1]  = -A[5*node+2]; // aE coefficient
            AA[node][node+nx] = -A[5*node+3]; // aN coefficient
            AA[node][node]    = A[5*node+4]; // aP coefficient
        }
        // Last node
        node = j * nx + nx - 1;
        AA[node][node-1] = -A[5*node+1];  // aW coefficient
        AA[node][node]   = A[5*node+4];  // aP coefficient
    }
    // // Upper row
    for(int i = 0; i < nx; i++) {
        int node = (ny - 1) * nx + i;
        AA[node][node-nx] = -A[5*node];   // aS coefficient
        AA[node][node]    = A[5*node+4]; // aP coefficient
    }
}

void assembleMatrix(const int nx, const int ny, const double* A, double* AA) {
    /*
    assembleMatrix: given the matrix A (is in vector form), the function assembles the matrix AA (square matrix) for LUP method. It is assumed that
    AA is already allocated.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - nx        Number of nodes in the X axis                               [const int]
        - ny        Number of nodes in the Y axis                               [const int]
        - A         Linear system matrix. Rows: nx*ny, Columns: 5               [const double*]
        - AA        Matrix to be assembled (in vector form)                     [double**]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs: temperature map of phi. Prior to running the program,
        - AA        Assembled matrix (in vector form)                           [double**]
    */
    int n = nx * ny;
    // // Lower row
    for(int i = 0; i < nx; i++) {
        AA[i*n+i]    = A[5*i+4];    // aP coefficient
        AA[i*n+i+nx] = -A[5*i+3];    // aN coefficient
    }
    // // Central rows
    for(int j = 1; j < ny-1; j++) {
        // First node
        int node = j * nx;
        AA[node*n+node] = A[5*node+4];    // aP coefficient
        AA[node*n+node+1] = -A[5*node+2];  // aE coefficient
        // Central nodes
        for(int i = 1; i < nx-1; i++) {
            node = j * nx + i;
            AA[node*n+node-nx] = -A[5*node];   // aS coefficient
            AA[node*n+node-1]  = -A[5*node+1]; // aW coefficient
            AA[node*n+node+1]  = -A[5*node+2]; // aE coefficient
            AA[node*n+node+nx] = -A[5*node+3]; // aN coefficient
            AA[node*n+node]    = A[5*node+4]; // aP coefficient
        }
        // Last node
        node = j * nx + nx - 1;
        AA[node*n+node-1] = -A[5*node+1];  // aW coefficient
        AA[node*n+node]   = A[5*node+4];  // aP coefficient
    }
    // // Upper row
    for(int i = 0; i < nx; i++) {
        int node = (ny - 1) * nx + i;
        AA[node*n+node-nx] = -A[5*node];   // aS coefficient
        AA[node*n+node]    = A[5*node+4]; // aP coefficient
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PRINT RESULTS FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void printToFile(const Mesh m, const double* phi, const char* filename, const int precision) {
    /*
    printToFile: prints the solution of the linear system to a file. When the file is open, the function creates a new file or overwrites the previous
    one if it already exists.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - m                 Mesh object                                                 [const Mesh]
        - phi               Solution of the linar system. Rows: nx*ny, Columns: 1       [const double*]
        - filename          Name of the file where the solution is to be written        [const char*]
        - precision         Number of decimal places to write doubles                   [const int]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs: the file where the solution has been written.
    */
    printf("Printing the solution to file '%s'...\n", filename);
    std::ofstream file;
    file.open(filename);
    if(file.is_open()) {
        printf("\tWriting to file...\n");
        file << std::setprecision(precision) << std::fixed;
        for(int i = 0; i < m.getNX(); i++) {
            for(int j = 0; j < m.getNY(); j++)
                file << m.atNodeX(i) << " " << m.atNodeY(j) << " " << phi[j*m.getNX()+i] << std::endl;
            file << std::endl;
        }
    } else
        printf("\tCould not open file\n");
    file.close();
    printf("\tClosing file...\n\n");
}

void plotSolution(const Mesh m, const char* filename) {
    /*
    plotSolution: plots phi vs (x,y) in a temperature map using gnuplot.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - m                 Mesh of the problem                                                 [const Mesh]
        - filename          Name of the file where the solution has been previously written     [const char*]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs: temperature map of phi. Prior to running the program,
        - the command "export DISPLAY=:0 gnuplot" must be executed (only needed before the first execution)
        - Xming server must be running
    */
    // Set xrange command
    char xrange[50];
    sprintf(xrange, "set xrange [%.5f : %.5f]", m.atNodeX(0), m.atNodeX(m.getNX()-1));

    // Set yrange command
    char yrange[50];
    sprintf(yrange, "set yrange [%.5f : %.5f]", m.atNodeY(0), m.atNodeY(m.getNY()-1));

    // Set size ratio command
    char sizeratio[50];
    sprintf(sizeratio, "set size ratio %.5f", (m.atNodeY(m.getNY()-1)-m.atNodeY(0))/(m.atNodeX(m.getNX()-1) - m.atNodeX(0)));

    // Plotting command, uses filename parameter
    char plotCommand[30+strlen(filename)];
    sprintf(plotCommand, "plot '%s' with image", filename);

    // Commands sent to gnuplot
    const char* GnuCommands[] = {"unset key",
        "set xlabel 'x (m)'",
        "set xtics format '%.1f'",
        "set xtics border out nomirror",
        "set ylabel 'y (m)'",
        "set ytics format '%.1f'",
        "set ytics border out nomirror",
        "set cblabel 'phi'",
        "set cbtics format '%.1f'",
        "set palette rgb 33,13,10",
        xrange, yrange, sizeratio, plotCommand};

    // Send commands to gnuplot
    FILE *gnupipe = NULL;
    gnupipe = popen("gnuplot -persistent", "w");
    printf("Plotting the solution in gnuplot... Commands:\n");
    for(int i = 0; i < 14; i++) {
        printf("\t%s\n", GnuCommands[i]);
        fprintf(gnupipe, "%s\n", GnuCommands[i]);
    }
    printf("\n");
}


void printVelocityField(const Mesh m, double (*vx)(double, double), double (*vy)(double, double), const char* filename_mod, const char* filename_vec, const int precision) {
    /*
    printVelocityField: prints the velocity field to a file. filename_mod contains the norm of the velocity field at each point. filename_vec contains
    the norm and the components of the velocity field at each point.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - m                 Mesh of the problem                                                                         [const Mesh]
        - filename          Name of the file where the solution has been previously written                             [const char*]
        - *vx               Function that gives the velocity in the x axis provided the (x,y) coordinates               [returns double, needs (double,double)]
        - *vy               Function that gives the velocity in the y axis provided the (x,y) coordinates               [returns double, needs (double,double)]
        - filename_mod      File where the norm of the velocity field at each point is printed.                         [const char*]
        - filename_vec      File where the norm and the components of the velocity field at each point are printed.     [const char*]
        - precision         Number of decimal places to which doubles are printed.                                      [const int]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - filename_mod      File where the norm of the velocity field at each point is printed.                         [const char*]
        - filename_vecd     File where the norm and the components of the velocity field at each point are printed.     [const char*]
    */
    // Print the (x,y) coordinates and the norm of the velocity field at (x,y)
    printf("Printing the velocity field norm...\n");
    std::ofstream file;
    file.open(filename_mod);
    if(file.is_open()) {
        printf("\tWriting to file '%s'...\n", filename_mod);
        file << std::setprecision(precision) << std::fixed;
        for(int j = 0; j < m.getNY(); j++) {
            for(int i = 0; i < m.getNX(); i++) {
                double x = m.atNodeX(i);
                double y = m.atNodeY(j);
                double vel = std::sqrt(vx(x,y)*vx(x,y) + vy(x,y)*vy(x,y));
                file << x << " " << y << " " << vel << std::endl;
            }
            file << std::endl;
        }
        file << std::endl;
    } else
        printf("\tCould not open file '%s'...\n", filename_mod);
    printf("\tClosing file...\n\n");
    file.close();

    // Print the (x,y) coordinates, the norm and the components of the velocity field at (x,y)
    printf("Printing the velocity field norm and components...\n");
    file.open(filename_vec);
    if(file.is_open()) {
        printf("\tWriting to file '%s'...\n", filename_vec);
        file << std::setprecision(precision) << std::fixed;
        for(int j = 0; j < m.getNY(); j++) {
            for(int i = 0; i < m.getNX(); i++) {
                double x = m.atNodeX(i);
                double y = m.atNodeY(j);
                double velx = vx(x,y);
                double vely = vy(x,y);
                double vel = std::sqrt(velx*velx + vely*vely);
                file << x << " " << y << " " << vel << " " << velx << " " << vely << std::endl;
            }
            file << std::endl;
        }
    } else
        printf("\tCould not open file '%s'...\n", filename_vec);
    printf("\tClosing file...\n\n");
    file.close();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHECK SOLUTION FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
double checkLinearSystemSolution(const int nx, const int ny, const double* A, const double* b, const double* phi) {
    /*
    checkLinearSystemSolution: checks the solution of the linear system resulting from the 2D convection-diffusion equations in a cartesian mesh. If _A is
    the linear system matrix, the function returns the infinity norm of _A * phi - b.
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Inputs:
        - nx        Number of nodes in the X axis                               [const int]
        - ny        Number of nodes in the Y axis                               [const int]
        - A         Linear system matrix. Rows: nx*ny, Columns: 5               [double*]
        - b         Vector of indenpendent terms. Rows: nx*ny, Columns: 1       [double*]
        - phi       Solution of the linar system. Rows: nx*ny, Columns: 1       [double*]
    --------------------------------------------------------------------------------------------------------------------------------------------------
    Outputs:
        - maxDiff   Infinity norm of the difference A * phi - b                 [double]
    */
    double maxDiff = -1;
    // Lower row nodes
    for(int i = 0; i < nx; i++) {
        double diff = b[i] + A[5*i+3] * phi[i+nx] - A[5*i+4] * phi[i];
        maxDiff = std::max(maxDiff, std::abs(diff));
    }
    // Central row nodes
    for(int j = 1; j < ny-1; j++) {
        for(int i = 0; i < nx; i++) {
            int node = j * nx + i;
            double diff = b[node] + (A[5*node]*phi[node-nx]) + (A[5*node+1]*phi[node-1]) + (A[5*node+2]*phi[node+1]) + (A[5*node+3]*phi[node+nx]) - (A[5*node+4]*phi[node]);
            maxDiff = std::max(maxDiff, std::abs(diff));
        }
    }
    // Upper row nodes
    for(int i = 0; i < nx; i++) {
        int node = (ny-1) * nx + i;
        double diff = b[node] + (A[5*node] * phi[node-nx]) - (A[5*node+4] * phi[node]);
        maxDiff = std::max(maxDiff, std::abs(diff));
    }
    return maxDiff;
}
