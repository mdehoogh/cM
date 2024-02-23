# testin heap's algorithm to generate all permutations interfactively

def generate(n,A):
  def swap(i,j):
    temp=A[i]
    A[i]=A[j]
    A[j]=temp
  result=[]
  c=[0]*n
  i=0 # instead of 1 as it states on Wikipedia
  # count=1
  result=[A[:]] # print("1: ",A)
  while i<n:
    if c[i]<i:
      if i%2==0: # even
       	swap(0,i)
      else: # odd
        swap(c[i],i)
      # count+=1
      result.append(A[:]) # print(count,": ",A)
      c[i]+=1
      i=0
    else:
      c[i]=0
      i+=1
  return result

gp4=generate(6,[1,2,3,4,5,6])
for i in range(720):
  print(i+1,':',gp4[i])

gp5=generate(5,[1,2,3,4,5])

# Python program to print all permutations using
# Heap's algorithm
 
# Generating permutation using Heap Algorithm
hp=[]
def heapPermutation(a, size):
  global hp
  # if size becomes 1 then prints the obtained
  # permutation
  if size == 1:
    hp.append(a[:]) # print(a)
    return
 
  for i in range(size):
    heapPermutation(a, size-1)
 
    # if size is odd, swap 0th i.e (first)
    # and (size-1)th i.e (last) element
    # else If size is even, swap ith
    # and (size-1)th i.e (last) element
    if size & 1:
      a[0], a[size-1] = a[size-1], a[0]
    else:
      a[i], a[size-1] = a[size-1], a[i]
 
 
# Driver code
a = [1,2,3,4,5]
n = len(a)
heapPermutation(a, n)
def eq(A,B,n):
  for k in range(n):
    if A[k]!=B[k]:
      return False
  return True
def diff(A,B,n):
  result=0
  for k in range(n):
    if A[k]!=B[k]:
      result+=1
  return result
for i in range(120):
  # determine for reproductivity
  equalindex1=0
  equalindex2=0
  for j in range(120):
    if i!=j:
      if equalindex1==0 and diff(gp5[i],gp5[j],5)==0:
        equalindex1=j+1
      if equalindex2==0 and diff(hp[i],hp[j],5)==0:
        equalindex2=j+1
  if i>0:
    d1=diff(gp5[i],gp5[i-1],5)
    d2=diff(hp[i],hp[i-1],5)
  else:
    d1='?'
    d2='?'
  print(i+1,':',gp5[i],'=',equalindex1,'>',d1,' ',hp[i],'=',equalindex2,'>',d2)

# This code is contributed by ankush_953
# This code was cleaned up to by more pythonic by glubs9
