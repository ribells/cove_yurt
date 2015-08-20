/*
	File:			VLArray.h

	Function:		Defines an VLArray type that manages its own storage space,
					and can be used as a stack or a list.
					
	Author(s):		Andrew Willmott

	Copyright:		(c) 1995-2000, Andrew Willmott
 */

#ifndef __VLArray__
#define __VLArray__

#include "cl/Basics.h"
#include <iostream>
#include <stdio.h>

#define TMPLVLArray	template<class T>
#define TVLArray		VLArray<T>

const Int kFirstAllocation = 16; // Default number of items to initially 
								 // allocate to the VLArray

TMPLVLArray class VLArray
{
public:
					VLArray();
					VLArray(Int size, Int alloc = kFirstAllocation);
					VLArray(const TVLArray &VLArray);
				   ~VLArray();
	
//	VLArray operators
	
	inline T		&operator [] (Int i);		///< Indexing operator
	inline const T	&operator [] (Int i) const; ///< Indexing operator
	inline Int		NumItems() const;			///< Number of items in the VLArray
	
	TVLArray			&operator = (const TVLArray &VLArray);	///< Assignment!
	
//	Useful for stack implementations

	inline T		&Top();						///< Return top of stack
	inline Void		Pop();						///< Delete top of stack
	inline Void		Push(const T &t);			///< Push item onto stack  
	
// List Operations

	inline Void		Append(const T &t);			///< Append single item to VLArray
	inline Void		Prepend(const T &t);		///< Prepend single item to VLArray
	inline T		&Last();					///< Return last item in VLArray
	Void			Clear();					///< Delete all items

	Void			PreAllocate(Int numItems);	///< Preallocate space for VLArray
	Void			SetSize(Int newSize);		///< Set VLArray size directly.
	Void			Add(Int n = 1);				///< Add n items to the VLArray
	Void			Shrink(Int n = 1);			///< shrink the VLArray by n items
	Void 			Insert(Int i, Int n = 1);	///< Insert n items at i
	Void			Delete(Int i, Int n = 1);	///< Delete n items at i
	Void			ShrinkWrap();				/**< Ensure allocated space =
													 space being used */
	Void			ClearTo(const T &t);		///< Clear the VLArray using t

	Void			Append(const TVLArray &a);	///< Append VLArray to VLArray
	Void			SwapWith(TVLArray &a);		///< Swaps this VLArray with a
	Void			Replace(TVLArray &a);			/**< Replace this VLArray with a
													 & clear a. */

	const T			&Item(Int i) const
					{ return(SELF[i]); };
	T				&Item(Int i)
					{ return(SELF[i]); };
	
// Low level access

	inline T		*Ref() const;				///< Return pointer to VLArray
	inline T		*Detach();					/**< As above, but the VLArray
													 no longer owns the data. */

	Void			Attach(T *itemsPtr, Int numItems, Bool shared);
					/**< Attach the VLArray to the given chunk of memory.
						 If shared is true, the memory won't be deleted 
						 when the VLArray is destroyed. */

	Void 			WriteFile(const Char *filename);
	Void 			ReadFile(const Char *filename);

	Int 			FWrite(FILE *file);
	Int 			FRead(FILE *file);

//	Private...

protected:
	T				*item;		///< pointer to VLArray
	Int 			items;		///< items in the VLArray
	Int				allocated;	///< number of items we have space allocated for. 
	
	Void 			Grow();
};	

TMPLVLArray ostream &operator << (ostream &s, TVLArray &VLArray);
TMPLVLArray istream &operator >> (istream &s, TVLArray &VLArray);


// --- Inlines ----------------------------------------------------------------


TMPLVLArray inline TVLArray::VLArray() : item(0), items(0), allocated(0)
{
}

TMPLVLArray inline Int TVLArray::NumItems() const
{
	return(items);
}

TMPLVLArray inline T &TVLArray::operator [] (Int i)
{
	CheckRange(i, 0, items, "(VLArray::[]) index out of range");

	return(item[i]);
}

TMPLVLArray inline const T &TVLArray::operator [] (Int i) const
{
	CheckRange(i, 0, items, "(VLArray::[]) index out of range");

	return(item[i]);
}

TMPLVLArray inline T &TVLArray::Top()
{
	return(item[items - 1]);
}

TMPLVLArray inline T &TVLArray::Last()
{
	return(item[items - 1]);
}

TMPLVLArray inline Void TVLArray::Push(const T &t)
{
	if (items >= allocated)
		Grow();
	
	item[items++] = t;
}

TMPLVLArray inline Void TVLArray::Append(const T &t)
{
	if (items >= allocated)
		Grow();
	
	item[items++] = t;
}

TMPLVLArray inline Void TVLArray::Prepend(const T &t)
{
	Insert(0, 1);
	item[0] = t;
}

TMPLVLArray inline Void TVLArray::Pop()
{	
	items--;
}

TMPLVLArray inline Void TVLArray::Clear()
{	
	items = 0;
	allocated = 0;
	delete[] item;
	item = 0;
}

TMPLVLArray inline T *TVLArray::Ref() const
{
	return(item);
}

TMPLVLArray inline T *TVLArray::Detach()
{
	T* result = item;

	items = 0;
	allocated = 0;
	item = 0;

	return(result);
}

TMPLVLArray inline Void TVLArray::ClearTo(const T &t)
{
	for (Int i = 0; i < items; i++)
		item[i] = t;
}

#endif
