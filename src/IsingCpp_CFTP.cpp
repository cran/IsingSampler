#include <Rcpp.h>
#include <climits>
using namespace Rcpp;

// FUNCTIONS FOR EXACT SAMPLING //

// Inner function to resize list:
List resize( const List& x, int n ){
    int oldsize = x.size() ;
    List y(n) ;
    for( int i=0; i<oldsize; i++) y[i] = x[i] ;
    return y ;
}

// Inner function to simulate random uniforms in a matrix:
NumericMatrix RandMat(int nrow, int ncol)
 {
  int N = nrow * ncol;
  NumericMatrix Res(nrow,ncol);
  NumericVector Rands  = runif(N);
   for (int i = 0; i < N; i++) 
  {
    Res[i] = Rands[i];
  }
  return(Res);
 }

// Computes maximal and minimal probability of node flipping:
NumericVector PplusMinMax(int i, NumericMatrix J, IntegerVector s, NumericVector h, double beta, IntegerVector responses)
{
  // The function computes the probability that node i is in Response 1 instead of 0, given all other nodes, which might be missing.
  // Output: minimal and maximal probablity
  
  NumericVector H0(2, h[i] * responses[0]); // relevant part of the Hamiltonian for state = 0
  NumericVector H1(2, h[i] * responses[1]); // relevant part of the Hamiltonian for state = 1
  
  NumericVector Res(2);
  
  int N = J.nrow();
  NumericVector TwoOpts(2);
  
  for (int j=0; j<N; j++)
  {
    if (i != j)
    {
      if (s[j] != INT_MIN)
      {
       H0[0] += J(i,j) * responses[0] * s[j];
       H0[1] += J(i,j) * responses[0] * s[j];
       H1[0] += J(i,j) * responses[1] * s[j];
       H1[1] += J(i,j) * responses[1] * s[j]; 
      } else 
      {
               
        TwoOpts[0] = J(i,j) * responses[1] * responses[0];
        TwoOpts[1] = J(i,j) * responses[1] * responses[1];

        if (TwoOpts[1] > TwoOpts[0])
        {
          H1[0] += TwoOpts[0];
          H1[1] += TwoOpts[1];
          
          H0[0] += J(i,j) * responses[0] * responses[0];
          H0[1] += J(i,j) * responses[0] * responses[1];
        } else 
        {
          H1[0] += TwoOpts[1];
          H1[1] += TwoOpts[0];          
          
          H0[0] += J(i,j) * responses[0] * responses[1];
          H0[1] += J(i,j) * responses[0] * responses[0];
        }
      }
    }
  }

  Res[0] = exp(beta * H1[0]) / ( exp(beta * H0[0]) + exp(beta * H1[0]) );
  Res[1] = exp(beta * H1[1]) / ( exp(beta * H0[1]) + exp(beta * H1[1]) );
  
  
  return(Res);
}
       
// Inner function:
IntegerVector IsingEx(NumericMatrix graph, NumericVector thresholds, double beta, int nIter, IntegerVector responses, bool exact)
{
  // Parameters and results vector:
  int N = graph.nrow();
  IntegerVector state(N, INT_MIN);
  double u;
  NumericVector P(2);
  int maxChain = 100;
  List U(1);
  int minT = 0;
  bool anyNA = true;
    
  do
  { 
    // Resize U if needed:
    if (minT > 0)
    {
      U = resize(U, minT+1);
    }
    
    // Generate new random numbers:
    U[minT] = RandMat(nIter, N);
    
    // Initialize states:
    for (int i=0; i<N; i++)
    {
      if (exact)
      {
        state[i] = INT_MIN;
      } else 
      {
        state[i] = ifelse(runif(1) < 0.5, responses[1], responses[0])[0];
      }
    }    

    // START ALGORITHM
    for (int t=minT; t > -1;  t--)
    {
      for (int it=0;it<nIter;it++)
      {
        NumericMatrix Ucur = U[t];
        for (int node=0;node<N;node++)
        {
          u = Ucur(it, node);
          P = PplusMinMax(node, graph, state, thresholds, beta, responses);
          if (u < P[0])
          {
            state[node] = responses[1];
          } else if (u >= P[1])
          {
            state[node] = responses[0];
          } else 
          {
            state[node] = INT_MIN;
          }
        }
      }
    }
    
    anyNA = false;
    if (exact)
    {
      if (minT < maxChain)
      {
       for (int i=0; i<N; i++)
       {
        if (state[i] == INT_MIN)
        {
          anyNA = true;
        }
        } 
      } 
    }    
    minT++;
    
  } while (anyNA);

  // Rf_PrintValue(wrap(minT));
  return(state);
}


// FUNCTIONS FOR METROPOLIS SAMPLER //
double Pplus(int i, NumericMatrix J, IntegerVector s, NumericVector h, double beta, IntegerVector responses)
{
  // The function computes the probability that node i is in Response 1 instead of 0, given all other nodes, which might be missing.
  // Output: minimal and maximal probablity
  
  double H0 = h[i] * responses[0]; // relevant part of the Hamiltonian for state = 0
  double H1 = h[i] * responses[1]; // relevant part of the Hamiltonian for state = 1
  
  int N = J.nrow();
  
  for (int j=0; j<N; j++)
  {
    if (i != j)
    {
       H0 += J(i,j) * responses[0] * s[j];
       H1 += J(i,j) * responses[1] * s[j];
    }
  }
  
  return(exp(beta * H1) / ( exp(beta * H0) + exp(beta * H1) ));
}


IntegerVector IsingMet(NumericMatrix graph, NumericVector thresholds, double beta, int nIter, IntegerVector responses)
{
  // Parameters and results vector:
  int N = graph.nrow();
  IntegerVector state =  ifelse(runif(N) < 0.5, responses[1], responses[0]);
  double u;
  double P;
    
    // START ALGORITHM
    for (int it=0;it<nIter;it++)
    {
      for (int node=0;node<N;node++)
      {
        u = runif(1)[0];
        P = Pplus(node, graph, state, thresholds, beta, responses);
        if (u < P)
        {
          state[node] = responses[1];
        } else 
        {
          state[node] = responses[0];
        }
      }
    }
   
  return(state);
}


///ISING PROCESS SAMPLER:
// [[Rcpp::export]]
IntegerMatrix IsingProcess(int nSample, NumericMatrix graph, NumericVector thresholds, double beta, IntegerVector responses)
{
  // Parameters and results vector:
  int N = graph.nrow();
  IntegerVector state =  ifelse(runif(N) < 0.5, responses[1], responses[0]);
  double u;
  double P;
  IntegerMatrix Res(nSample,N);
  int node;
    
    // START ALGORITHM
    for (int it=0;it<nSample;it++)
    {
      node = floor(R::runif(0,N));
        u = runif(1)[0];
        P = Pplus(node, graph, state, thresholds, beta, responses);
        if (u < P)
        {
          state[node] = responses[1];
        } else 
        {
          state[node] = responses[0];
        }
        for (int k=0; k<N; k++) Res(it,k) = state[k];
    }
   
  return(Res);
}

// OVERAL FUNCTION //
// [[Rcpp::export]]
IntegerMatrix IsingSamplerCpp(int n, NumericMatrix graph, NumericVector thresholds, double beta, int nIter, IntegerVector responses, bool exact)
{
  int Ni = graph.nrow();
  IntegerMatrix Res(n,Ni);
  IntegerVector state(Ni);
  if (exact)
  {
    for (int s=0;s<n;s++)
    {
      state = IsingEx(graph, thresholds, beta, nIter, responses, exact);
      for (int i=0;i<Ni;i++) Res(s,i) = state[i];
    }
  } else 
  {
    for (int s=0;s<n;s++)
    {
      state = IsingMet(graph, thresholds, beta, nIter, responses);
      for (int i=0;i<Ni;i++) Res(s,i) = state[i];
    }
  }
  
  return(Res);
}


// HELPER FUNCTIONS //
// Hamiltonian:
// [[Rcpp::export]]
double H(NumericMatrix J, IntegerVector s, NumericVector h)
{
  double Res = 0;
  int N = J.nrow();
  for (int i=0;i<N;i++)
  {
    Res -= h[i] * s[i];
    for (int j=i;j<N;j++)
    {
      if (j!=i) Res -= J(i,j) * s[i] * s[j];
    }
  }
  return(Res);
}


// Likelihood without Z
// [[Rcpp::export]]
double f(IntegerMatrix Y, NumericMatrix J, NumericVector h)
{
  double Res = 1;
  int Np = Y.nrow();
  int Ni = J.ncol();
  IntegerVector s(Ni);
  for (int p=0;p<Np;p++)
  {
    for (int i=0;i<Ni;i++)
    {
      s[i] = Y(p,i);
    }
    Res *= exp(-1.0 * H(J, s, h));
  }
  return(Res);
}


// VECTOR VERSIONS //


// Hamiltonian:
// [[Rcpp::export]]
double Hvec(IntegerVector s, NumericVector Theta, int N)
{
  double Res = 0;
  int c=0;
  for (int i=0;i<N;i++)
  {
    Res -= Theta[c] * s[i];
    c++;
  }
  for (int i=0;i<N;i++)
  {
    for (int j=i; j<N;j++)
    {
     if (j!=i) 
     {
       Res -= Theta[c] * s[i] * s[j]; 
       c++;
     }
    }
  }
  return(Res);
}

// Likelihood without Z
double fvec(IntegerMatrix Y, NumericVector Theta)
{
  double Res = 1;
  int Np = Y.nrow();
  int Ni = Y.ncol();
  IntegerVector s(Ni);
  for (int p=0;p<Np;p++)
  {
    for (int i=0;i<Ni;i++)
    {
      s[i] = Y(p,i);
    }
    Res *= exp(-1.0 * Hvec(s, Theta, Ni));
  }
  return(Res);
}

// Log-Likelihood without Z
double fveclog(IntegerMatrix Y, NumericVector Theta)
{
  double Res = 1;
  int Np = Y.nrow();
  int Ni = Y.ncol();
  IntegerVector s(Ni);
  for (int p=0;p<Np;p++)
  {
    for (int i=0;i<Ni;i++)
    {
      s[i] = Y(p,i);
    }
    Res -= Hvec(s, Theta, Ni);
  }
  return(Res);
}

IntegerMatrix vecSampler(int n, int N, NumericVector Theta, int nIter, IntegerVector responses)
{
   NumericVector thresh(N);
   for (int i=0; i<N; i++)
   {
     thresh[i] = Theta[i];
   }
   
   NumericMatrix graph(N,N);
   int c=N+1;
   for (int i=0;i<N;i++)
   {
    for (int j=i; j<N;j++)
    {
     if (j!=i) 
     {
       graph(i,j) = Theta[c];
      graph(j,i) = Theta[c];
       c++;
     }
    }
  }
   
   return(IsingSamplerCpp(n, graph, thresh, 1.0, nIter, responses, true));
}

// Uniform distribution (prior):
double FakeUnif(NumericVector x, double lower, double upper)
{
  double Res = 1.0;
  
  for (int i=0; i < x.length(); i++)
  {
    if (x[i] < lower || x[i] > upper)
    {
      Res = 0.0;
      break;
    }
  }
  
  return(Res);
}

/* 
// Progress bar function:
int progress_bar(double x, double N)
{
    // how wide you want the progress meter to be
    int totaldotz=40;
    double fraction = x / N;
    // part of the progressmeter that's already "full"
    int dotz = round(fraction * totaldotz);

    // create the "meter"
    int ii=0;
    printf("%3.0f%% [",fraction*100);
    // part  that's full already
    for ( ; ii < dotz;ii++) {
        printf("=");
    }
    // remaining part (spaces)
    for ( ; ii < totaldotz;ii++) {
        printf(" ");
    }
    // and back to line begin - do not forget the fflush to avoid
    // output buffering problems!
    printf("]\r");
    fflush(stdout);
}
*/ 

// EXCHANGE ALGORTIHM //
// [[Rcpp::export]]
NumericMatrix ExchangeAlgo(IntegerMatrix Y, double lowerBound, double upperBound, double stepSize, int nIter, IntegerVector responses,
    bool simAn, double tempStart, double tempEnd, NumericVector StartValues)
{
  int Np = Y.nrow();
  int Ni = Y.ncol();
  
  // Number of parameters:
  int Npar = Ni + (Ni*(Ni-1))/2;
  
  // Fantasy matrix:
  IntegerMatrix X(Np,Ni);
  
  // Results matrix:
  NumericMatrix Samples(nIter, Npar);
  
  // Current parameter values:
  // NumericVector curPars = runif(Npar, lowerBound, upperBound);
  NumericVector curPars(Npar, 0.0);
  for (int i=0; i<Npar; i++) curPars[i] = StartValues[i];
  NumericVector propPars(Npar);
  
  double a;
  double r;
  
  // START ITERATING //
  for (int it=0; it<nIter; it++)
  {
   // progress_bar((double)it, (double)nIter);
    // For each parameter:
    for (int n=0;n<Npar;n++)
    {
      // Propose new state:
      for (int i=0; i<Npar; i++)
      {
        if (i==n)
        {
          propPars[i] = curPars[i] + R::rnorm(0.0,stepSize);
        } else 
        {
          propPars[i] = curPars[i];
        }
      }
      
      // Simulate data with new state:
      X =  vecSampler(Np, Ni, propPars, nIter, responses);
      
      // Random number:
      r = R::runif(0,1);
      
      // Acceptance probability:
      a = FakeUnif(propPars,lowerBound,upperBound)/FakeUnif(curPars,lowerBound,upperBound) * 
           exp(fveclog(Y, propPars) + fveclog(X, curPars) - fveclog(Y,curPars) - fvec(X,propPars));
      
      if (!simAn)
      {
        if (r < a)
        {
          curPars[n] = propPars[n];
        }  
      } else {
        if (r < exp(log(a)/ (tempStart - it * (tempStart-tempEnd)/nIter)))
        {
          curPars[n] = propPars[n];
        }  
      }
      
      
      Samples(it, n) = curPars[n];
    }
  }
  
  
  return(Samples);
}

