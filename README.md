Log
---
5 August 2019

cM is my first serious C programming enterprise, so ...

About a year ago I wrote a Python version of my interpreter, which I called M (M of Marc or Marcia or Miscellaneous or Miraculous or ...) and stopped working on it at the moment that it allowed me to write a Chess application in it.

This year I started writing the C version of M and it's taking a long long time.

However, by now (5 September 2019) it's starting to look like something, though there's still a long way to go.
That's mainly due to the fact that I wanted to be able to use big integers, then rationals, and also decimals.
For big integers I use the (sources of the) tommath library, and for decimals (the sources of) the mpdecimal library (as used in Python 3), the rationals I do myself.

Right now I'm working on implementing sine and cosine functions to work with decimals, as somehow mpdecimal doesn't do that (although it does square root much appreciated thanks), that's kinda fun.
Interestingly, given the sine and cosine identity relationship, you might not need to have to use series at all to compute them.
The interesting equalities are the sin(a+b), and the sin(a/2). Now, knowing the sine and cosine of 45 degrees and 30 degrees I suppose you can compute the sine and cosine of any angle (under 90 degrees which is all we need btw).

With 45 you can get 22.5 etc. etc. etc. as small as you'd want them to be. Obviously, with any given angle you just have to find the sequence of pluses and minuses of these elementary angles that you know the sine and cosines of,
and you're done. I guess CORDIC is like that, although CORDIC goes about it backwards. I suppose if you use recursion, I'd first locate the nearest angle that we know about, then we ask for the sine and cosine of the difference
which also looks for the nearest angle, when that one returns the formula can be used to compute the sine or cosine of the sum or difference, so obviously it won't be using the formulas until actually having computed the sine or
cosine of the smallest angle with a series expansion somehow, which will be faster if the final angle is very small. At that point the recursion ends and all the formulas are getting applied up and up and up until done. So this
going up is like applying the CORDIC rotations, except that CORDIC is iterative, and not recursive. Of course the granularity is important as well, if we only store 30 and 60, i.e. two angles the largest distance will be 15 degrees,
if we store the sine/cosine of 15, 30, 45, 60, 75 degrees, the smallest distance will be 7.5. CORDIC stores the tangents of 45 degrees, 22.5 degrees and lower, until the smallest possible angle with the required accuracy; that way
no series expansion is ever necessary. This is all fine with a single precision. Of course the sine of a small angle is close to that angle as we know that sin(x)/x approaches 1 if x approaches 0, so even with any finite arbitrary
precision as with my decimals, you can keep halving until you're below the accuracy of the decimal precision. Of course, you can't substitute zero at that point, you'll end up with zero as result. I'm gonna think a little more 
about this problem.

(to be continued)
