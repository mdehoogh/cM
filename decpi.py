import decimal
import sys

def pi(module, prec):
	"""From the decimal.py documentation"""
	module.getcontext().prec = prec + 2
	D = module.Decimal
	lasts, t, s, n, na, d, da = D(0), D(3), D(3), D(1), D(0), D(0), D(24)
	iter=0
	print("Iteration "+str(iter)+": lasts="+str(lasts)+", t="+str(t)+", s="+str(s)+", n="+str(n)+", na="+str(na)+", d="+str(d)+", da="+str(da)+".")
	while s != lasts:
		#print("\tlasts="+str(lasts)+", t="+str(t)+", s="+str(s)+", n="+str(n)+", na="+str(na)+", d="+str(d)+", da="+str(da)+".")
		#print("Iteration "+str(iter)+":")
		lasts = s
		#print("\tlasts="+str(s)+"(s)")
		n, na = n+na, na+8
		#print("\tn="+str(n)+", na="+str(na))
		d, da = d+da, da+32
		#print("\td="+str(n)+", da="+str(na))
		t = (t * n) / d
		#print("\t="+str(t))
		s += t
		#print("\ts="+str(s)+"(s)+"+str(t)+"(t)")
		iter+=1
		print("Iteration "+str(iter)+": lasts="+str(lasts)+", t="+str(t)+", s="+str(s)+", n="+str(n)+", na="+str(na)+", d="+str(d)+", da="+str(da)+".")
	module.getcontext().prec -= 2
	return +s

def main(args):
	if len(args)>0:
		precision=int(args[0])
	else:
		precision=28
	y = pi(decimal, precision)
	print("Pi in "+str(precision)+" decimals: "+str(y)+".")

if __name__=="__main__":
	main(sys.argv[1:])