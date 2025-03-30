"""
compute the total number of combinations for a 3 x 3 matrix 
let's say it is [a b c, 
                 d e f,
                 g h i]
and one of the determinant formulas equals aei-afh+bfg-bdi+cdh-ceg
so if we call these elements 1 through 9 the determinant is the sum
of 6 three element terms like this
"""
## replacing: productindices=[[1,5,9],[1,6,8],[2,6,7],[2,4,9],[3,4,8],[3,5,7]]
variable_names=["a","b","c","d","e","f","g","h","i"]
product_names=["aei","afh","bfg","bdi","cdh","ceg"]
products_flags=[ \
[1,0,0,0,1,0,0,0,1], \
[1,0,0,0,0,1,0,1,0], \
[0,1,0,0,0,1,1,0,0], \
[0,1,0,1,0,0,0,0,1], \
[0,0,1,1,0,0,0,1,0], \
[0,0,1,0,1,0,1,0,0]]
# let's iterate over all possible (+,-) combinations a total of 2**9=512
import sys
productsigncounts=[0]*64 # there are a total of 64 possible combinations
for signs in range(0,512):
	if signs<10:
		sys.stdout.write('  ')
	elif signs<100:
		sys.stdout.write(' ')
	sys.stdout.write(str(signs))
	sys.stdout.write(' ')
	signbits=[0]*9
	for i in range(0,9):
		signbits[i]=(signs>>(8-i))&1 # either 0 or 1
		sys.stdout.write(' ')
		sys.stdout.write(variable_names[i])
		sys.stdout.write('=')
		sys.stdout.write(str(signbits[i]))
	# let determine the signs of each of the six combinations
	# 0 means + and 1 means -
	# can we get the bits?????
	productsigns=[0]*6
	# computing the inproduct of each of the product flags row
	for (productindex,product_flags) in enumerate(products_flags):
		#signbyte=0
		sign=0 # this is the sign for the product
		sys.stdout.write(' ')
		for i in range(0,9):
			contribution=(signbits[i]&product_flags[i])
			sys.stdout.write(str(contribution))
			sign+=contribution
		productsigns[productindex]=sign
		# signbyte indicates the array of signs for a given product
	for productindex in range(0,6):
		sys.stdout.write(' ')
		sys.stdout.write(product_names[productindex])
		sys.stdout.write('=')
		sys.stdout.write(str(productsigns[productindex]))
	productsign=0
	for i in range(0,6):
		productsign=(productsign<<1)+(productsigns[i]&1)
	sys.stdout.write(' ')
	sys.stdout.write(str(productsign))
	productsigncounts[productsign]+=1
	sys.stdout.write(' ')
	print(str(productsigncounts[productsign]))
# let's display it from 0 through 31 and from 63 to 32
print('product sign counts: ')
for i in range(0,32):
	sys.stdout.write(' ')
	if i<10:
		sys.stdout.write(' ')
	sys.stdout.write(str(i))
	sys.stdout.write('=')
	sys.stdout.write(str(productsigncounts[i]))
print()
for i in range(63,31,-1):
	sys.stdout.write(' ')
	sys.stdout.write(str(i))
	sys.stdout.write('=')
	sys.stdout.write(str(productsigncounts[i]))
print()
print(sum(productsigncounts))
