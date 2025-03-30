"""
compute the total number of combinations for a 3 x 3 matrix 
let's say it is [a b c, 
                 d e f,
                 g h i]
and one of the determinant formulas equals aei-afh+bfg-bdi+cdh-ceg
so if we call these elements 1 through 9 the determinant is the sum
of 6 three element terms like this
"""
productindices=[[1,5,9],[1,6,8],[2,6,7],[2,4,9],[3,4,8],[3,5,7]]
# let's iterate over all possible (+,-) combinations a total of 2**9=512
import sys
productsigncounts=[0]*64 # there are a total of 64 possible combinations
for signs in range(0,512):
	# let determine the signs of each of the six combinations
	# 0 means + and 1 means -
	# can we get the bits?????
	productsigns=[0]*6
	for i in range(1,10):
		signbit=(signs>>(9-i))&1 # either 0 or 1
		sys.stdout.write(str(signbit))
		if signbit: # we have to toggle the sign bits
			for (index,productindex) in enumerate(productindices):
				for j in range(0,3):
					if productindex[j]==i:
						productsigns[index]=1-productsigns[index]
	sys.stdout.write(' ')
	sys.stdout.write(str(productsigns))
	productsign=0
	for i in range(0,6):
		productsign=(productsign<<1)+productsigns[i]
	productsigncounts[productsign]+=1
	sys.stdout.write(' ')
	sys.stdout.write(str(productsign))
	sys.stdout.write(' ')
	print(str(productsigncounts[productsign]))
print("The product sign counts: ",productsigncounts)
print(sum(productsigncounts))
