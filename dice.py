# brute force 3 dice problem

combinations=((0,1,2),(0,2,1),(1,0,2),(1,2,0),(2,0,1),(2,1,0))

# I have to do 6 loops over all iterations
# depending on what's on a side of a die we can know how much it contributes to the sum
contributions=(0,0,0,1,1,2,4,4,6,9,9,12,16,16,20,25,25,30,36)

def getCombinations(dice):
 counts=[0]*3
 d=[0]*18
 for die in range(0,3):
  for x in dice[die]:
   d[x-1]=die
 for i in range(1,17):
  for j in range(i+1,18):
   for k in range(j+1,19):
    counts[d[k-1]]+=1
 return counts
permindex=0
successes=0
for a in range(0,6):
 ca=combinations[a]
 for b in range(0,6):
  cb=combinations[b]
  for c in range(0,6):
   cc=combinations[c]
   for d in range(0,6):
    cd=combinations[d]
    for e in range(0,6):
     ce=combinations[e]
     for f in range(0,6):
      cf=combinations[f]
      permindex+=1
      dice=[[],[],[]]
      # for each number to assign
      for i in range(0,3):
       dice[ca[i]].append(i+1)
      for i in range(0,3):
       dice[cb[i]].append(i+4)
      for i in range(0,3):
       dice[cc[i]].append(i+7)
      for i in range(0,3):
       dice[cd[i]].append(i+10)
      for i in range(0,3):
       dice[ce[i]].append(i+13)
      for i in range(0,3):
       dice[cf[i]].append(i+16)
      # all 3 dice determined, and the numbers increment
      # we should now be able to compute the three sums
      # which we want to all equal 72
      die=0
      sums=[0,0,0]
      while die<3:
       sums[die]=0
       for i in range(0,6):
        sums[die]+=contributions[dice[die][i]]
       if sums[die]!=72:
         break
       die+=1
      if die==3: # a solution found
       #sys.stdout.write(str(permindex))
       successes+=1
       print("Success #",successes," at permutation #",permindex," with sums ",sums,': ',dice,getCombinations(dice)) 
print("All ",permindex," permutations processed.")
