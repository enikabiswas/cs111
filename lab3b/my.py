#!/usr/bin/python

import sys, string, locale, csv

class superblock :
	def __init__(self, csv):
		self.total_blocks = int(csv[1])
		self.total_inodes = int(csv[2])
		self.block_size = int(csv[3])
		self.inode_size = int(csv[4])
		self.first_nonreserved_inode = int(csv[7])

'''
class block {
	block_number = 0
	ref_list = []
}
'''
class inode :
	#ref_list = []
	def __init__(self, csv):
		self.inode_num = int(csv[1])
		self.type = csv[2]
		self.link_count = int(csv[6])
		self.blocks = []
		for i in range(12,24):
			self.blocks.append(int(csv[i]))
		self.single_indirect = int(csv[24])
		self.double_indirect = int(csv[25])
		self.triple_indirect = int(csv[26])


class dirent :
	def __init__(self, csv):
		self.parent_inode = int(csv[1])
		self.offset = int(csv[2])
		self.inode_of_refd = int(csv[3])
		self.entry_len = int(csv[4])
		self.name_len = int(csv[5])
		self.name = csv[6]


class indirect :
	def __init__(self, csv):
		self.owner_inode_num = int(csv[1])
		self.level = int(csv[2])
		self.offset_of_refd = int(csv[3])
		self.scanned_block_num = int(csv[4])
		self.refd_block_num = int(csv[5])


if __name__ == '__main__':
	#for exit
	code = 0
	i_free = []
	b_free = []
	indirects = []
	dirents = []
	inodes = {}
	blocks = {}

	if len(sys.argv) != 2:
		sys.stderr.write("*** Error: incorrect number of arguments!")

	try:
		file = open(sys.argv[1], "r")
	except:
		sys.stderr.write("*** Error: unable to open file!")

	with file as csvfile:
		reader = csv.reader(csvfile, delimiter=',', quotechar="\'")
		for line in reader:
			if line[0] == "SUPERBLOCK":
				sup_block = superblock(line)
			elif line[0] == "BFREE":
				b_free.append(line[1])
			elif line[0] == "IFREE":
				i_free.append(line[1])
			elif line[0] == "DIRENT":
				dirents.append(dirent(line))
			elif line[0] == "INDIRECT":
				indirects.append(indirect(line))
			elif line[0] == "INODE":
				newinode = inode(line)
				inodes[line[1]] = newinode #key'd based on inode number => line[1]


	################ BLOCK CONSISTENCY AUDITS ######################

	for i in inodes:
		offset = 0
		for b in i.blocks:
			if b != 0:
				if b < 0 or b >= sup_block.total_blocks:
					print("INVALID BLOCK {} IN INODE {} AT OFFSET {}".format(b, i.inode_num, offset))
					code = 2
				elif b < 11: #first non reserved inode
					print("RESERVED BLOCK {} IN INODE {} AT OFFSET {}".format(b, i.inode_num, offset))
					code = 2
				elif b in blocks:
					blocks.append((i.inode_num, offset, 0)) #tuple format: inode number, offset, level of indirection
				else:
					blocks[b] = [(i.inode_num, offset, 0)] #if list doesn't already exist, then make a new list 
			offset += 1

		offset = 12 #these were calcluated in my previous lab
		if i.single_indirect != 0:
			if i.single_indirect < 0 or i.single_indirect >= sup_block.total_blocks:
				print("INVALID INDIRECT BLOCK {} IN INODE {} AT OFFSET {}".format(b, i.inode_num, offset))
				code = 2
			elif i.single_indirect < 11:
				print("RESERVED INDIRECT BLOCK {} IN INODE {} AT OFFSET {}".format(b, i.inode_num, offset))
				code = 2
			elif b in blocks:
				blocks.append((i.inode_num, offset, 1)) #tuple format: inode number, offset, level of indirection
			else:
				blocks[b] = [(i.inode_num, offset, 1)] #if list doesn't already exist, then make a new list 

		offset = 268 #these were calcluated in my previous lab
		if i.double_indirect != 0:
			if i.double_indirect < 0 or i.double_indirect >= sup_block.total_blocks:
				print("INVALID DOUBLE INDIRECT BLOCK {} IN INODE {} AT OFFSET {}".format(b, i.inode_num, offset))
				code = 2
			elif i.double_indirect < 11:
				print("RESERVED DOUBLE INDIRECT BLOCK {} IN INODE {} AT OFFSET {}".format(b, i.inode_num, offset))
				code = 2
			elif b in blocks:
				blocks.append((i.inode_num, offset, 2)) #tuple format: inode number, offset, level of indirection
			else:
				blocks[b] = [(i.inode_num, offset, 2)] #if list doesn't already exist, then make a new list 

		offset = 65804 #these were calcluated in my previous lab
		if i.triple_indirect != 0:
			if i.triple_indirect < 0 or i.triple_indirect >= sup_block.total_blocks:
				print("INVALID TRIPLE INDIRECT BLOCK {} IN INODE {} AT OFFSET {}".format(b, i.inode_num, offset))
				code = 2
			elif i.triple_indirect < 11:
				print("RESERVED TRIPLE INDIRECT BLOCK {} IN INODE {} AT OFFSET {}".format(b, i.inode_num, offset))
				code = 2
			elif b in blocks:
				blocks.append((i.inode_num, offset, 3)) #tuple format: inode number, offset, level of indirection
			else:
				blocks[b] = [(i.inode_num, offset, 3)] #if list doesn't already exist, then make a new list 


	#we should now have all of the blocks saved into our blocks dictionary
	for b in range(11, sup_block.total_blocks):
		if b not in b_free and b not in blocks:
			print("UNREFERENCED BLOCK {}".format(b))
			code = 2
		elif b in blocks and b in b_free:
			print("ALLOCATED BLOCK {} ON FREELIST".format(b))
			code = 2
		if b in blocks and len(blocks[b]) > 1:
			message = ""
			if blocks[b][2] == 1:
				message = "INDIRECT"
			elif blocks[b][2] == 2:
				message = "DOUBLE INDIRECT "
			elif blocks[b][2] == 3:
				message = "TRIPLE INDIRECT "
			
			print("DUPLICATE {}BLOCK {} IN INODE {} AT OFFSET {}".format(message, b, blocks[b][0], blcoks[b][1]))
			code = 2


	###############################################################





	################ INODE ALLOCATION AUDITS ######################
'''
	for i in range (2, sup_block.total_inodes):
		if i in inodes and i in i_free:
			print("ALLOCATED INODE {} ON FREELIST".format(i))
			code = 2
		elif i not in inodes and i not in i_free:
			print("UNALLOCATED INODE {} NOT ON FREELIST".format(i))
			code = 2
'''
	for i in inodes:
		if i.type == '0': #because we don't cast this to an int
			if i.inode_num not in i_free:
				print("ALLOCATED INODE {} ON FREELIST".format(i.inode_num))
				code = 2
		elif i.type != '0':
			if i.inode_num in i_free:
				print("UNALLOCATED INODE {} NOT ON FREELIST".format(i.inode_num))
				code = 2

	for i in range(sup_block.first_nonreserved_inode, sup_block.total_inodes):
		if i not in inodes and i not in i_free:
			print("UNALLOCATED INODE {} NOT ON FREELIST".format(i))
			code = 2
	###############################################################





	################ DIRECTORY ALLOCATION AUDITS ##################

	links = [0] * sup_block.total_inodes #https://stackoverflow.com/questions/10712002/create-an-empty-list-in-python-with-certain-size

	for d in dirents:
		index = d.inode_of_refd;
		links[index] += 1
		if d.inode_of_refd in i_free:
			print("DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}".format(d.parent_inode, d.name, d.inode_of_refd))
			code = 2
		if d.inode_of_refd < 0 or d.inode_of_refd > sup_block.total_inodes:
			print("DIRECTORY INODE {} NAME {} INVALID INODE {}".format(d.parent_inode, d.name, d.inode_of_refd))
			code = 2

	for l in range(2, sup_block.total_inodes):
		if l in inodes and links[l] != inodes[l].link_count:
			print("INODE {} HAS {} LINKS BUT LINKCOUNT IS {}".format(l, links[l], inodes[l].link_count))
			code = 2
		if l in inodes and links[l] == 0:
			print("INODE {} HAS 0 LINKS BUT LINKCOUNT IS {}".format(l, inodes[l].link_count))
			code = 2




	###############################################################

	exit(code)







