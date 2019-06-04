#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>

// number of terms in continued fraction.
// 15 is the max without precision errors for M_PI
#define MAX 100
#define eps 1e-20

// MDH@04JUN2019: if we're only interested in the p and the q we can do the following
void find_pq(long double x,int maxiter,long double maxeps,long long *_p,long long *_q){
  if(x==0){
    *_p=0;
    *_q=1;
    return;
  }
  if(x<0){
    find_pq(-x,maxiter,maxeps,_p,_q);
    *_p=-*_p; // negate the numerator
    return;
  }
  // x is positive 
  long long pmin1=1,pmin2=0,qmin1=0,qmin2=1;
  long long a,p,q;
  long double delta,rem=x;
  for(int i=1;i<=MAX;i++){
    a=lrint(floorl(rem));
    p=a*pmin1+pmin2;
    q=a*qmin1+qmin2;
    printf("\nIteration #%u: %lld:  %lld/%lld", i, a, p, q);
    rem-=a;
    delta=(x*q)-p;
    printf(" - delta: %.*Lf, rem: %.*Lf",LDBL_DIG,delta,LDBL_DIG,rem);
    if(fabsl(rem)<maxeps)return;
    if(fabsl(delta)<maxeps){printf(" ******* 'true' deviation below epsilon threshold");return;}
    rem=1/rem;
    // shift the lot
    pmin2=pmin1;qmin2=qmin1;
    pmin1=p;qmin1=q;
  }
  *_p=p;
  *_q=q;
}

long long p[MAX], q[MAX], a[MAX], len;
void find_cf(long double x) {
  int i;
  //The first two convergents are 0/1 and 1/0
  p[0] = 0; q[0] = 1;
  p[1] = 1; q[1] = 0;
  //The rest of the convergents (and continued fraction)
  for(i=2; i<MAX; ++i) {
    a[i] = lrint(floorl(x));
    p[i] = a[i]*p[i-1] + p[i-2];
    q[i] = a[i]*q[i-1] + q[i-2];
    printf("%lld:  %lld/%lld\n", a[i], p[i], q[i]);
    len = i;
    if(fabsl(x-a[i])<eps) return;// if close enough to zero we're done
    x = 1.0/(x - a[i]);
  }
}

void all_best(long double x) {
  find_cf(x); printf("\n");
  int i; long long n, cp, cq;
  for(i=2; i<len; ++i) {
    //Test n = a[i+1]/2. Enough to test only when a[i+1] is even, actually...
    n = a[i+1]/2; cp = n*p[i]+p[i-1]; cq = n*q[i]+q[i-1];
    if(fabsl(x-(long double)cp/cq) < fabsl(x-(long double)p[i]/q[i])) 
      printf("%lld/%lld, ", cp, cq);
    //And print all the rest, no need to test
    for(n = (a[i+1]+2)/2; n<=a[i+1]; ++n) {
      printf("%lld/%lld, ", n*p[i]+p[i-1], n*q[i]+q[i-1]);
    }
  }
}

int main(int argc, char **argv) {
  long double x;
  if(argc==1) { x = M_PI; } else { sscanf(argv[1], "%Lf", &x); }
  assert(x>0); printf("%.15Lf\n\n", x);
  find_cf(x);
  long long num,den;
  find_pq(x,MAX,eps,&num,&den); printf("\nMy results: %lld/%lld.",num,den);
  // all_best(x); printf("\n");
  return 0;
}