//! \file exceptions.hpp Defines fastcgi++ exceptions
/***************************************************************************
* Copyright (C) 2007 Eddie Carle [eddie@mailforce.net]                     *
*                                                                          *
* This file is part of fastcgi++.                                          *
*                                                                          *
* fastcgi++ is free software: you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as  published   *
* by the Free Software Foundation, either version 3 of the License, or (at *
* your option) any later version.                                          *
*                                                                          *
* fastcgi++ is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public     *
* License for more details.                                                *
*                                                                          *
* You should have received a copy of the GNU Lesser General Public License *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.       *
****************************************************************************/

#ifndef SQL_HPP
#define SQL_HPP

#include <vector>
#include <list>
#include <queue>
#include <cstring>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <fastcgi++/message.hpp>

//! A empty parameters set placeholder for use with Fastcgipp::Sql::Statement::queue and it's children
#define EMPTY_SQL_SET boost::shared_ptr<Fastcgipp::Sql::Data::Set>()
//! A empty result set placeholder for use with Fastcgipp::Sql::Statement::queue and it's children
#define EMPTY_SQL_CONT boost::shared_ptr<Fastcgipp::Sql::Data::SetContainerPar>()
//! A empty rows/insertId integer placeholder for use with Fastcgipp::Sql::Statement::queue and it's children
#define EMPTY_SQL_INT boost::shared_ptr<unsigned long long int>()

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	//! Defines classes and functions relating to SQL querying
	namespace Sql
	{
		//! Defines data types and conversion techniques standard to the fastcgipp %SQL facilities.
		namespace Data
		{
			/** 
			 * @brief Defines data types supported by the fastcgi++ sql facilities.
			 *
			 * This enumeration provides runtime type identification capabilities to classes derived from
			 * the Set class. All types starting with U_ mean unsigned and all types ending will _N means
			 * they can store null values via the Nullable class.
			 */
			enum Type { U_TINY=0,
							U_SHORT,
							U_INT,
							U_BIGINT,
							TINY,
							SHORT,
							INT,
							BIGINT,
							FLOAT,
							DOUBLE,
							TIME,
							DATE,
							DATETIME,
							BLOB,
							TEXT,
							WTEXT,
							CHAR,
							BINARY,
							BIT,		
							U_TINY_N,
							U_SHORT_N,
							U_INT_N,
							U_BIGINT_N,
							TINY_N,
							SHORT_N,
							INT_N,
							BIGINT_N,
							FLOAT_N,
							DOUBLE_N,
							TIME_N,
							DATE_N,
							DATETIME_N,
							BLOB_N,
							TEXT_N,
							WTEXT_N,
							CHAR_N,
							BINARY_N,
							BIT_N,
							NOTHING };
			
			/** 
			 * @brief Base class to the Nullable template class.
			 *
			 * This base class provides a polymorphic method of retrieving a void pointer to the contained
			 * object regardless of it's type along with it's nullness.
			 *
			 * If nullness is true then the value is null.
			 */
			struct NullablePar
			{
				NullablePar(bool _nullness): nullness(_nullness) { }
				bool nullness;
				/** 
				 * @brief Retrieve a void pointer to the object contained in the class.
				 * 
				 * @return Void pointer to the object contained in the class.
				 */
				virtual void* getVoid() =0;
			};
			
			/** 
			 * @brief Class for adding null capabilities to any type. Needed for SQL queries involving
			 * null values.
			 */
			template<class T> struct Nullable: public NullablePar
			{
				T object;
				void* getVoid() { return &object; }
				operator T() { return object; }
				Nullable(): NullablePar(false) {}
				Nullable(const T& x): NullablePar(false), object(x) { }
			};

			template<class T, int size> struct NullableArray: public NullablePar
			{
				T object[size];
				void* getVoid() { return object; }
				operator T*() { return object; }
				NullableArray(): NullablePar(false) {}
				NullableArray(const T& x): NullablePar(false), object(x) { }
			};

			/** 
			 * @brief A basic, practically none-functional stream inserter for Nullable objects.
			 */
			template<class charT, class Traits, class T> inline std::basic_ostream<charT, Traits>& operator<<(std::basic_ostream<charT, Traits>& os, const Nullable<T>& x)
			{
				if(x.nullness)
					os << "NULL";
				else
					os << x.object;

				return os;
			}

			typedef unsigned char Utiny;
			typedef signed char Tiny;
			typedef unsigned short int Ushort;
			typedef short int Short;
			typedef unsigned int Uint;
			typedef int Int;
			typedef unsigned long long int Ubigint;
			typedef long long int Bigint;
			typedef float Float;
			typedef double Double;
			typedef boost::posix_time::time_duration Time;
			typedef boost::gregorian::date Date;
			typedef boost::posix_time::ptime Datetime;
			typedef std::vector<char> Blob;
			typedef std::string Text;
			typedef std::wstring Wtext;
			//typedef std::bitset Bit;

			typedef Nullable<unsigned char> UtinyN;
			typedef Nullable<char> TinyN;
			typedef Nullable<unsigned short int> UshortN;
			typedef short int Short;
			typedef Nullable<unsigned int> UintN;
			typedef Nullable<int> IntN;
			typedef Nullable<unsigned long long int> UbigintN;
			typedef Nullable<long long int> BigintN;
			typedef Nullable<float> FloatN;
			typedef Nullable<double> DoubleN;
			typedef Nullable<boost::posix_time::time_duration> TimeN;
			typedef Nullable<boost::gregorian::date> DateN;
			typedef Nullable<boost::posix_time::ptime> DatetimeN;
			typedef Nullable<std::vector<char> > BlobN;
			typedef Nullable<std::string> TextN;
			typedef Nullable<std::wstring> WtextN;
			//typedef Nullable<std::bitset> BitN;

			/** 
			 * @brief Base data set class for communicating parameters and results with SQL queries.
			 *
			 * By deriving from this class any data structure can gain the capability to be binded to
			 * the parameters or results of an SQL query. This is accomplished polymorphically through three
			 * virtual member functions that allow the object to be treated as a container and it's member
			 * data indexed as it's elements. An example derivation follows:
@code
struct TestSet: public Fastcgipp::Sql::Data::Set
{
	size_t numberOfSqlElements() const { return 7; }
	Fastcgipp::Sql::Data::Type getSqlType(size_t index) const
	{
		switch(index)
		{
			case 0:
				return Fastcgipp::Sql::Data::DOUBLE_N;
			case 1:
				return Fastcgipp::Sql::Data::DATE_N;
			case 2:
				return Fastcgipp::Sql::Data::TIME;
			case 3:
				return Fastcgipp::Sql::Data::DATETIME_N;
			case 4:
				return Fastcgipp::Sql::Data::WTEXT_N;
			case 5:
				return Fastcgipp::Sql::Data::BLOB_N;
			case 6:
				return Fastcgipp::Sql::Data::BINARY;
			default:
				return Fastcgipp::Sql::Data::NOTHING;
		}
	}

	const void* getConstPtr(size_t index) const
	{
		switch(index)
		{
			case 0:
				return &fraction;
			case 1:
				return &aDate;
			case 2:
				return &aTime;
			case 3:
				return &timestamp;
			case 4:
				return &someText;
			case 5:
				return &someData;
			case 6:
				return fixedChunk;
			default:
				return 0;
		}
	}

	size_t getSqlSize(size_t index) const
	{
		switch(index)
		{
			case 6:
				return sizeof(fixedChunk);
			default:
				return 0;
		}
	}

	Fastcgipp::Sql::Data::DoubleN fraction;
	Fastcgipp::Sql::Data::DateN aDate;
	Fastcgipp::Sql::Data::Time aTime;
	Fastcgipp::Sql::Data::DatetimeN timestamp;
	Fastcgipp::Sql::Data::WtextN someText;
	Fastcgipp::Sql::Data::BlobN someData;
	char fixedChunk[16];
};
@endcode
			 * Note that the indexing order must match the result column/parameter order of the
			 * SQL query.
			 *
			 * All bindable types in the class should be of a type that is typedefed in Fastcgipp::Sql::Data
			 * (don't worry, they are all standard types). Each of these typedefs has a corresponding value
			 * in the Fastcgipp::Sql::Data::Type enumeration for return from getSqlType().
			 *
			 * @sa Fastcgipp::Sql::Data::Nullable
			 */
			struct Set
			{
				/** 
				 * @brief Get total number of indexable data members.
				 * 
				 * @return Total number of indexable data members.
				 */
				virtual size_t numberOfSqlElements() const =0;

				/** 
				 * @brief Get type associated with particular index value.
				 * 
				 * @param[in] index Index number for member, starting at 0.
				 * 
				 * @return Associated type;
				 */
				virtual Type getSqlType(size_t index) const =0;

				/** 
				 * @brief Get size associated with particular index value.
				 *
				 * This virtual function need only be defined in the event of custom
				 * binary data structures. Anything of fixed length like an array or
				 * some sort of struct. It will only be called for types that identify
				 * themselves as Fastcgipp::Sql::Data::BINARY, Fastcgipp::Sql::Data::CHAR
				 * or their nullable equivalents.
				 * 
				 * @param[in] index Index number for member, starting at 0.
				 * 
				 * @return Size of fixed data chunk in bytes.
				 */
				virtual size_t getSqlSize(size_t index) const { }

				/** 
				 * @brief Get constant void pointer to member data.
				 * 
				 * @param[in] index index Index number for member, starting at 0.
				 * 
				 * @return Constant void pointer to member data.
				 */
				const void* getSqlPtr(size_t index) const { return getConstPtr(index); }

				/** 
				 * @brief Get non-constant void pointer to member data.
				 * 
				 * @param[in] index index Index number for member, starting at 0.
				 * 
				 * @return Non-constant void pointer to member data.
				 */
				void* getSqlPtr(size_t index) { return const_cast<void*>(getConstPtr(index)); }
			protected:
				/** 
				 * @brief Get constant void pointer to member data.
				 * 
				 * @param[in] index index Index number for member, starting at 0.
				 * 
				 * @return Constant void pointer to member data.
				 */
				virtual const void* getConstPtr(size_t index) const =0; 
			};

			/** 
			 * @brief Base class to to SetContainer.
			 */
			class SetContainerPar
			{
			public:
				typedef std::list<boost::shared_ptr<Set> > Cont;
			protected:
				Cont data;

			public:
				virtual Cont::iterator manufacture() =0;
				//Set& getSetEnd() { return *data.back(); }
				void trim() { data.erase(--data.end()); }

				size_t size() const { return data.size(); }
				bool empty() const { return data.empty(); }
			};

			/** 
			 * @brief Container class for Set objects.
			 *
			 * This class defines a basic container for types derived from the Set class.
			 *	It is intended for retrieving multi-row results from SQL queries. It is based
			 *	on a linked list concept allowing efficient insertion of data but no random
			 *	access.
			 */
			template<class T> class SetContainer: public SetContainerPar
			{
			public:
				class iterator
				{
				private:
					Cont::iterator it;
					friend class SetContainer;
					iterator(const Cont::iterator it_): it(it_) {}
				public:
					iterator() {}
					iterator(const iterator& it_): it(it_.it) {}
					T& operator*() const { return *(T*)it->get(); }
					T* operator->() const { return (T*)it->get(); }
					iterator& operator++() { ++it; return *this; } 
					iterator& operator--() { --it; return *this; } 
					iterator operator++(int) { return iterator(it++); } 
					iterator operator--(int) { return iterator(it--); } 
					bool operator==(const iterator& x) const { return x.it==it; }
					bool operator!=(const iterator& x) const { return x.it!=it; }
					iterator& operator=(const iterator& x) { it=x.it; return *this; }
				};

				class const_iterator
				{
				private:
					Cont::const_iterator it;
					friend class SetContainer;
					const_iterator(Cont::const_iterator it_): it(it_) {}
				public:
					const_iterator() {}
					const_iterator(const const_iterator& it_): it(it_.it) {}
					const T& operator*() const { return *(T*)it->get(); }
					const T* operator->() const { return (const T*)it->get(); }
					const_iterator& operator++() { ++it; return *this; } 
					const_iterator& operator--() { --it; return *this; } 
					const_iterator operator++(int) { return const_iterator(it++); } 
					const_iterator operator--(int) { return const_iterator(it--); } 
					bool operator==(const const_iterator& x) const { return x.it==it; }
					bool operator!=(const const_iterator& x) const { return x.it!=it; }
					const_iterator& operator=(const const_iterator& x) { it=x.it; return *this; }
				};

				iterator begin() { return iterator(data.begin()); }
				iterator end() { return iterator(data.end()); }
				const_iterator begin() const { return const_iterator(data.begin()); }
				const_iterator end() const { return const_iterator(data.end()); }
				T& front() { return *(T*)data.front().get(); }
				T& back() { return *(T*)data.back().get(); }
				const T& front() const { return *(T*)data.front().get(); }
				const T& back() const { return *(T*)data.back().get(); }
				/** 
				 * @brief Allows the base class to append objects of the correct type.
				 * 
				 * @return Base class iterator pointing to the newly create object.
				 */
				Cont::iterator manufacture() { data.push_back(boost::shared_ptr<Set>(new T)); return --data.end(); }
			};
		
			/** 
			 * @brief Handle data conversion from standard data types to internal SQL engine types.
			 */
			struct Conversion
			{
				/** 
				 * @brief Get a pointer to the internal data.
				 * 
				 * @return Void pointer to internal data.
				 */
				virtual void* getPointer() =0;

				/** 
				 * @brief Convert SQL query results.
				 */
				virtual void convertResult() =0;

				/** 
				 * @brief Convert SQL query parameters.
				 */
				virtual void convertParam() =0;

				/** 
				 * @brief Pointer to standard data type.
				 */
				void* external;
			};
			
			typedef std::map<int, boost::shared_ptr<Conversion> > Conversions;
		}

		/** 
		 * @brief %SQL %Connection.
		 */
		class Connection
		{
		protected:
			/** 
			 * @brief Type value to use when sending Message structures back from asynchronous queries.
			 */
			const int typeVal;

			/** 
			 * @brief Number of threads to pool for simultaneous queries.
			 */
			const int maxThreads;
			boost::mutex threadsMutex;
			boost::condition_variable threadsChanged;
			int threads;

			boost::condition_variable wakeUp;

			boost::mutex terminateMutex;
			bool terminateBool;

			Connection(const int typeVal_, const int maxThreads_): typeVal(typeVal_), maxThreads(maxThreads_), threads(0) {}
		public:
			virtual ~Connection() { }
			/** 
			 * @brief Start a thread pool to handle queued asynchronous queries.
			 */
			virtual void start() =0;
			/** 
			 * @brief Terminate all queue handling threads.
			 */
			virtual void terminate() =0;
		};

		template<class T> class ConnectionPar: private Connection
		{
		private:
			/** 
			 * @brief Structure for storing information about queued queries.
			 */
			struct Query
			{
				T* statement;
				boost::shared_ptr<Data::Set> parameters;
				boost::shared_ptr<Data::SetContainerPar> results;
				boost::shared_ptr<unsigned long long int> insertId;
				boost::shared_ptr<unsigned long long int> rows;
				boost::function<void (Fastcgipp::Message)> callback;
			};

			/** 
			 * @brief Thread safe queue of queries.
			 */
			class Queries: public std::queue<Query>, public boost::mutex {} queries;

			/** 
			 * @brief Function that runs in threads.
			 */
			void intHandler();

		protected:
			ConnectionPar(const int typeVal_, const int maxThreads_): Connection(typeVal_, maxThreads_) {}
			inline void queue(T* const& statement, const boost::shared_ptr<Data::Set>& parameters, const boost::shared_ptr<Data::SetContainerPar>& results, const boost::shared_ptr<unsigned long long int>& insertId, const boost::shared_ptr<unsigned long long int>& rows, const boost::function<void (Message)>& callback);
		public:
			virtual ~ConnectionPar() { }
			void start();
			void terminate();
		};	

		/** 
		 * @brief SQL %Statement.
		 */
		class Statement
		{
		protected:
			Data::Conversions paramsConversions;
			Data::Conversions resultsConversions;
		public:
			/** 
			 * @brief Execute %SQL statement.
			 *
			 *	Executes the built query with the passed parameter data storing the results in the passed results container.
			 *	The number of rows affected or total matching rows can be retrieved by passing the proper pointer to rows.
			 *	If SQL_CALC_FOUND_ROWS is included in a select statement then this value will represent the total matching rows
			 *	regardless of a LIMIT clause. The last insert value on a auto-incremented column can also be retrieved with an
			 *	appropriate pointer in insertId.
			 *
			 *	The Data::Set pointed to by parameters must have the same derived type as was passed upon construction of the
			 *	statement. A null pointer, as in the constructor, indicates no parameter data. The results parameter should be
			 *	a pointer to a Data::SetContainer templated to the same derived type passed upon construction of the statement
			 *	for the result set. As well, a null pointer indicates no result data.
			 * 
			 * @param[in] parameters %Data set of SQL query parameter data.
			 * @param[out] results %Data set container of SQL query result data.
			 * @param[out] insertId Pointer to integer for writing of last auto-increment insert value.
			 * @param[out] rows Pointer to integer for writing the number of rows affected from last query.
			 */
			virtual void execute(Data::Set* const parameters, Data::SetContainerPar* const results, unsigned long long int* const insertId, unsigned long long int* const rows) =0;

			/** 
			 * @brief Asynchronously execute a %SQL statement.
			 *
			 * This function will queue the statement to be executed in a separate thread and return immediately. The information for
			 * execute() applies here with a few minor differences. For one, shared pointers are passed to prevent data being destroyed
			 * in one thread before it is finished with in another (segfault). So don't cheat, make sure they are shared pointer controlled
			 * on your end as well.
			 *
			 * For two, a callback function is replied that matches up nicely with the one provided in
			 * Fastcgipp::Request::callback. The data passed in the message is likely of a type Exceptions::CodedException.
			 * 
			 * @param[in] parameters %Data set of SQL query parameter data. If no data pass SQL_EMPTY_SET.
			 * @param[out] results %Data set container of SQL query result data. If no data pass SQL_EMTPY_CONT.
			 * @param[out] insertId Pointer to integer for writing of last auto-increment insert value. If not needed pass SQL_EMPTY_INT
			 * @param[out] rows Pointer to integer for writing the number of rows affected/matching from last query. If not needed pass SQL_EMPTY_INT.
			 * @param[in] callback Callback function taking a Message parameter.
			 */
			virtual void queue(const boost::shared_ptr<Data::Set>& parameters, const boost::shared_ptr<Data::SetContainerPar>& results, const boost::shared_ptr<unsigned long long int>& insertId, const boost::shared_ptr<unsigned long long int>& rows, const boost::function<void (Message)>& callback) =0;
			virtual ~Statement() { }
		};
	}
}

template<class T> void Fastcgipp::Sql::ConnectionPar<T>::start()
{
	{
		boost::lock_guard<boost::mutex> terminateLock(terminateMutex);
		terminateBool=false;
	}
	
	boost::unique_lock<boost::mutex> threadsLock(threadsMutex);
	while(threads<maxThreads)
	{
		boost::thread(boost::bind(&Fastcgipp::Sql::ConnectionPar<T>::intHandler, boost::ref(*this)));
		threadsChanged.wait(threadsLock);
	}
}

template<class T> void Fastcgipp::Sql::ConnectionPar<T>::terminate()
{
	{
		boost::lock_guard<boost::mutex> terminateLock(terminateMutex);
		terminateBool=true;
	}
	wakeUp.notify_all();

	boost::unique_lock<boost::mutex> threadsLock(threadsMutex);
	while(threads)
		threadsChanged.wait(threadsLock);
}

template<class T> void Fastcgipp::Sql::ConnectionPar<T>::intHandler()
{
	{
		boost::lock_guard<boost::mutex> threadsLock(threadsMutex);
		++threads;
	}
	threadsChanged.notify_one();
	
	boost::unique_lock<boost::mutex> terminateLock(terminateMutex, boost::defer_lock_t());
	boost::unique_lock<boost::mutex> queriesLock(queries, boost::defer_lock_t());

	while(1)
	{
		terminateLock.lock();
		if(terminateBool)
			break;
		terminateLock.unlock();

		queriesLock.lock();
		if(!queries.size())
		{
			wakeUp.wait(queriesLock);
			queriesLock.unlock();
			continue;
		}

		Query query(queries.front());
		queries.pop();
		queriesLock.unlock();

		Message message;
		message.type=typeVal;
		message.size=0;

		try
		{
			query.statement->execute(query.parameters.get(), query.results.get(), query.insertId.get(), query.rows.get());
		}
		catch(std::exception& e)
		{
			message.size=std::strlen(e.what()+1);
			message.data.reset(new char[message.size]);
			std::memcpy(message.data.get(), e.what(), message.size);
		}

		query.callback(message);
	}

	{
		boost::lock_guard<boost::mutex> threadsLock(threadsMutex);
		--threads;
	}
	threadsChanged.notify_one();
}

template<class T> void Fastcgipp::Sql::ConnectionPar<T>::queue(T* const& statement, const boost::shared_ptr<Data::Set>& parameters, const boost::shared_ptr<Data::SetContainerPar>& results, const boost::shared_ptr<unsigned long long int>& insertId, const boost::shared_ptr<unsigned long long int>& rows, const boost::function<void (Message)>& callback)
{
	boost::lock_guard<boost::mutex> queriesLock(queries);
	queries.push(Query());

	Query& query=queries.back();
	query.statement=statement;
	query.parameters=parameters;
	query.results=results;
	query.insertId=insertId;
	query.rows=rows;
	query.callback=callback;

	wakeUp.notify_one();
}

#endif