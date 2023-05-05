# testin heap's algorithm to generate all permutations interfactively

def generate(n,A):
  def swap(i,j):
    temp=A[i]
    A[i]=A[j]
    A[j]=temp
  c=[0]*n
  i=1
  count=1
  print("1: ",A)
  while i<n:
    if c[i]<i:
      if i%2:
       	swap(0,i)
      else:
        swap(c[i],i)
      count+=1
      print(count,": ",A)
      c[i]+=1
      i=1
    else:
      c[i]=0
      i+=1

generate(5,[1,2,3,4,5])
