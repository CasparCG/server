#ifndef _ID_H_
#define _ID_H_

namespace caspar
{
	namespace utils
	{
		class ID 
		{ 			
		public:
			typedef long long value_type;

			ID();
			const value_type& Value() const;
			static ID Generate(void* ptr);
		private:
			value_type value_; 
		};

		class Identifiable
		{
		public:
			Identifiable();
			virtual ~Identifiable(){}
			const utils::ID& ID() const;
		private:
			const utils::ID id_;
		};

		bool operator==(const ID& lhs,const ID& rhs);
	}
}

#endif