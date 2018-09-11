f = open("res.txt")
line1 = f.readline()
line2 = f.readline()
line3 = f.readline()
line4 = f.readline()

items1 = line1.split(', ')
items2 = line2.split(', ')
items3 = line3.split(', ')
items4 = line4.split(', ')

build_par = float(items1[5][7:])
build_seq = float(items3[5][7:])
build_spd = build_seq/build_par

query_par = float(items2[5][7:])
query_seq = float(items4[5][7:])
query_spd = query_seq/query_par


fout = open('data.txt','w')
fout.write(items1[0]+', '+items1[3]+', '+items1[4]+', ')
fout.write("sequential time = " + str(build_seq) + ", parallel time = " + str(build_par) + ", speedup = " + str(build_spd)+'\n')
fout.write(items2[0]+', '+items2[3]+', '+items2[4]+', ')
fout.write("sequential time = " + str(query_seq) + ", parallel time = " + str(query_par) + ", speedup = " + str(query_spd)+'\n')
fout.close()




