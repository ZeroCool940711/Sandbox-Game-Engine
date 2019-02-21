/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef UNDOREDO_HPP
#define UNDOREDO_HPP

#include <string>
#include <vector>
#include "cstdmf/smartpointer.hpp"

/**
 *	This class keeps track of undo and redo stuff
 */
class UndoRedo
{
public:
	UndoRedo();
	~UndoRedo();

	bool canUndo() const;
	bool canRedo() const;

	/**
	 *	This class is one undo operation between undo/redo barriers.
	 *	Two operations are considered equal if they refer to the
	 *	same data on the same object, and are replacing not compounding
	 *
	 *	Use int(typeid(*this).name()) for the 'kind' of such ops.
	 */
	class Operation
	{
	public:
		Operation( int kind ) : kind_( kind ) { }
		virtual ~Operation() { }

		virtual void undo() = 0;

		bool operator==( const Operation & oth ) const
		{
			if (kind_ != oth.kind_ || !kind_) return false;
			return this->iseq( oth );
		};

		bool operator!=( const Operation & oth ) const
		{
			return !( *this == oth );
		}

		virtual bool iseq( const Operation & oth ) const = 0;

	protected:
		int	kind_;
	};

	typedef std::vector<Operation*>	Operations;


	/**
	 *	This method adds an operation to the current undo or redo list,
	 *	depending on what's happening at the time. The UndoRedo object
	 *	takes ownership of the object passed in (i.e. deletes it when done)
	 */
	void add( Operation * op )
	{
		if (!undoing_)
			this->addUndo( op );
		else
			this->addRedo( op );
	}

	void undo();
	void redo();

	void barrier( const std::string & what, bool skipIfNoChange );

	const std::string & undoInfo( uint32 level ) const;
	const std::string & redoInfo( uint32 level ) const;

	/** Clears both the undo and redo lists */
	void clear();

	static UndoRedo & instance();

private:
	UndoRedo( const UndoRedo& );
	UndoRedo& operator=( const UndoRedo& );

	void addUndo( Operation * op );
	void addRedo( Operation * op );

	// when changes are made, 'addUndo' is called
	// when changes are undone by 'undo', 'addRedo' is called
	// when changes are redone by 'redo', 'addUndo' is called again

	void barrierInternal( const std::string & what );
	void clearRedos();

	class Barrier : public ReferenceCount
	{
	public:
		~Barrier();

		std::string	what_;
		Operations	ops_;
	};
	
	typedef SmartPointer<Barrier> BarrierPtr;
	typedef std::vector<BarrierPtr> Barriers;

	Barriers	undoList_;	// back is additions since latest barrier
	Barriers	redoList_;	// back is next Operations to redo
							//  (except during undo when it's additions)
	bool		undoing_;
};


#ifdef CODE_INLINE
#include "undoredo.ipp"
#endif

#endif // UNDOREDO_HPP
