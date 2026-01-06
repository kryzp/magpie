#pragma once

#include "container/string.h"

/*
 * TODO: Unfinished.
 */

class ClassDB {
public:
	template <class C>
	static void add_class()
	{
	}
};

#define DB_CLASS_BASE(_class) 						        				\
private:											        				\
	friend class ClassDB;					        						\
public:												        				\
	virtual String get_class() const {   									\
		return String(#_class);             		  						\
	}                                    			        				\
	static String get_class_static() { 										\
		return String(#_class);              								\
	}                 								        				\
    virtual bool is_class(const String &c) const { 							\
    	return String(#_class) == c;                 						\
	}                                    			        				\
    static void get_inheritance_list_static(Vector<String> *list) {       	\
	} 														                \
	static void init_class() {           			        				\
    	static bool is_initialized = false;   		        				\
		if (is_initialized)													\
			return;                             	        				\
		ClassDB::add_class<_class>();		        						\
		is_initialized = true; 						        				\
	} 												        				\
private:

#define DB_CLASS(_class, _inherits)										    \
private:																    \
	friend class ClassDB;          								            \
public:                               									    \
	virtual String get_class() const {   							        \
		return String(#_class);             							    \
	}                                    								    \
	static String get_class_static() { 								        \
		return String(#_class);              						        \
	}                                    								    \
	static String get_parent_class_static() { 						        \
		return _inherits::get_class_static(); 						        \
	}                                    							        \
    virtual bool is_class(const String &c) const { 					        \
    	return (c == String(#_class)) ? true : _inherits::is_class(c);      \
	}                                    								    \
    static void get_inheritance_list_static(Vector<String> *list) {       	\
		_inherits::get_inheritance_list_static(list);    	                \
		list->push_back(String(#_class));					                \
	} 														                \
	static void init_class() {           					                \
    	static bool is_initialized = false;   				                \
		if (is_initialized)													\
			return;                             			                \
		_inherits::init_class();            				                \
		ClassDB::add_class<_class>();				               			\
		is_initialized = true;												\
	} 														                \
private:
