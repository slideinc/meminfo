'''meminfo.py

A grab bag of tools for looking at memory usage primarily using the
garbage collection (gc) modules hooks for object tracking.
'''
import sys
import gc
import itertools

import _meminfo

LIMIT = 10000

sameframe = lambda a, b: a.f_code.co_filename == b.f_code.co_filename

def dump_loop(obj, limit = LIMIT, verbose = False):
	'''dump_loop

	Walk an objects referrers and if a loop back to this object is found,
	in the object walk limit provided, return a list of objects that are
	a part of the loop in referent order.

	The result is a tuple containing; a) the number of objects examined,
	b) the number of objects left to examing based on the curremt working
	set, c) the level to which the search decended, d) a list countaining
	the objects that comprise the loop if one exists
	'''
	count  = 0
	level  = 0
	search = [obj]
	next   = len(search)
	path   = {}
	frame  = sys._getframe()
	ripper = lambda x: filter(
		lambda j: not isinstance(j, type(frame)) or not sameframe(j, frame),
		filter(
			lambda i: i not in [locals(), globals(), search, gc.garbage],
			gc.get_referrers(x)))

	for data in search:
		push = ripper(data)
		base = len(search)

		path.update(dict(itertools.izip(
			xrange(base, base + len(push)),
			itertools.repeat(count, len(push)))))

		if data is obj and count:
			result = []
			step = count

			while step:
				result.append(search[step])
				step = path[step]

			return count, len(search), level, result

		search.extend(push)
		del(push)

		count += 1
		if count == next:
			level += 1
			next = len(search)
			if verbose:
				print count, next, level

		if len(search) > limit:
			break

	return count, len(search), level, []

obj  = _meminfo.obj
tree = _meminfo.tree

memcalc = {
	dict:  lambda r,o,u,m,s: s and r or r+((m+1)*o),
	list:  lambda r,o,u,a: r + (o * a),
	tuple: lambda r,o,u,a: r + (o * a),
	}

sum_tuple = lambda *a: tuple(map(sum, zip(*a)))

def total_mem(t, f):
	return reduce(
		lambda j, k: j + f(*obj(k)),
		[0] + filter(lambda i: isinstance(i, t), gc.get_objects()))

total_mem_dict  = lambda: total_mem(
	dict, lambda r,o,u,m,s: s and r or r+((m+1)*o))
total_mem_list  = lambda: total_mem(
	list, lambda r,o,u,a: r + (o * a))
total_mem_tuple = lambda: total_mem(
	tuple, lambda r,o,u,a: r + (o * a))


mem_dict_terse = lambda *i: (i[2], i[3], float(i[2])/i[3], 1)

obj_info_dict = lambda: reduce(
	lambda i, j: sum_tuple(i, mem_dict_terse(*obj(j))),
	[(0,0,0,0)] + filter(lambda i: isinstance(i, dict), gc.get_objects()))

def memtree(*args, **kwargs):
	'''memtree

	wrapper for _meminfo.tree

    Return the (estimated) memory usage of the given object tree.
	'''
	return _meminfo.tree(args, kwargs.get('max_depth', 20))
