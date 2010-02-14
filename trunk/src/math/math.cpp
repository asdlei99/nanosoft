
#include <math.h>
#include <nanosoft/math.h>

#include <string>
#include <stdio.h>

namespace nanosoft
{
	/**
	* Функция-константа
	*/
	class MathConst: public MathFunctionImpl
	{
	private:
		/**
		* Значение константы
		*/
		double c;
	public:
		/**
		* Конструктор
		*/
		MathConst(double value): c(value) { }
		
		/**
		* Вычисление функции
		*/
		double eval() { return c; }
		
		/**
		* Вернуть производную функции
		*/
		MathFunction derive(const MathVar &var) { return new MathConst(0); }
		
		/**
		* Вернуть в виде строки
		*/
		std::string toString() {
			char buf[80];
			int len = sprintf(buf, "%f", c);
			return std::string(buf, len);
		}
	};
	
	/**
	* Класс переменной
	*/
	class MathVarImpl: public MathFunctionImpl
	{
	private:
		/**
		* Имя переменной
		*/
		const char *name;
		
		/**
		* Значение переменной
		*/
		double value;
	public:
		/**
		* Конструктор переменной
		*/
		MathVarImpl(const char *n, double v);
		
		/**
		* Деструктор переменной
		*/
		~MathVarImpl();
		
		/**
		* Вернуть название переменной
		*/
		const char *getName();
		
		/**
		* Вернуть значение переменной
		*/
		double getValue();
		
		/**
		* Установить значение переменной
		*/
		void setValue(double v);
		
		/**
		* Вычислить значение переменной
		*/
		double eval();
		
		/**
		* Вернуть производную
		*/
		MathFunction derive(const MathVar &var);
		
		/**
		* Вернуть в виде строки
		*/
		std::string toString();
	};
	
	/**
	* Конструктор переменной
	*/
	MathVarImpl::MathVarImpl(const char *n, double v)
	{
		name = n;
		value = v;
	}
	
	/**
	* Деструктор переменной
	*/
	MathVarImpl::~MathVarImpl()
	{
	}
	
	/**
	* Вернуть название переменной
	*/
	const char * MathVarImpl::getName()
	{
		return name;
	}
	
	/**
	* Вернуть значение переменной
	*/
	double MathVarImpl::getValue()
	{
		return value;
	}
	
	/**
	* Установить значение переменной
	*/
	void MathVarImpl::setValue(double v)
	{
		value = v;
	}
	
	/**
	* Вычислить значение переменной
	*/
	double MathVarImpl::eval()
	{
		return value;
	}
	
	/**
	* Вернуть производную
	*/
	MathFunction MathVarImpl::derive(const MathVar &var)
	{
		return new MathConst(var == this ? 1 : 0);
	}
	
	/**
	* Вернуть в виде строки
	*/
	std::string MathVarImpl::toString()
	{
		return name;
	}
	
	/**
	* Конструктор по умолчанию
	*/
	MathFunctionImpl::MathFunctionImpl()
	{
		ref_count = 0;
	}
	
	/**
	* Виртуальный деструктор
	*/
	MathFunctionImpl::~MathFunctionImpl()
	{
	}
	
	/**
	* Увеличить счетчик ссылок
	*/
	void MathFunctionImpl::lock()
	{
		ref_count++;
	}
	
	/**
	* Уменьшить счетчик ссылок, если станет 0, то самоудалиться
	*/
	void MathFunctionImpl::release()
	{
		ref_count--;
		if ( ref_count == 0 ) delete this;
	}
	
	/**
	* Конструктор
	*/
	MathVar::MathVar()
	{
		var = 0;
	}
	
	/**
	* Конструктор
	*/
	MathVar::MathVar(const char *name, double value)
	{
		var = new MathVarImpl(name, value);
		var->lock();
	}
	
	/**
	* Конструктор копий
	*/
	MathVar::MathVar(const MathVar &v)
	{
		var = v.var;
		var->lock();
	}
	
	/**
	* Деструктор
	*/
	MathVar::~MathVar()
	{
		if ( var ) var->release();
	}
	
	/**
	* Вернуть название переменной
	*/
	const char * MathVar::getName() const
	{
		return var->getName();
	}
	
	/**
	* Вернуть значение переменной
	*/
	double MathVar::getValue() const
	{
		return var->getValue();
	}
	
	/**
	* Установить значение переменной
	*/
	void MathVar::setValue(double v)
	{
		var->setValue(v);
	}
	
	/**
	* Вернуть функцию f(x) = x
	*/
	MathVar::operator MathFunction ()
	{
		return MathFunction(var);
	}
	
	/**
	* Функция f(x) = -x
	*/
	class MathNeg: public MathFunctionImpl
	{
	private:
		MathFunction a;
	public:
		MathNeg(MathFunction A): a(A) { }
		double eval() { return - a.eval(); }
		MathFunction derive(const MathVar &var) { return new MathNeg(a.derive(var)); }
		
		/**
		* Вернуть в виде строки
		*/
		std::string toString() {
			return "-(" + a.toString() + ")";
		}
	};
	
	/**
	* Функция F(x,y) = x + y
	*/
	class MathSum: public MathFunctionImpl
	{
	private:
		MathFunction a;
		MathFunction b;
	public:
		MathSum(const MathFunction &A, const MathFunction &B): a(A), b(B) { }
		double eval() { return a.eval() + b.eval(); }
		MathFunction derive(const MathVar &var) { return a.derive(var) + b.derive(var); }
		
		/**
		* Вернуть в виде строки
		*/
		std::string toString() {
			return a.toString() + " + " + b.toString();
		}
	};
	
	/**
	* Функция F(x,y) = x - y
	*/
	class MathSub: public MathFunctionImpl
	{
	private:
		MathFunction a;
		MathFunction b;
	public:
		MathSub(const MathFunction &A, const MathFunction &B): a(A), b(B) { }
		double eval() { return a.eval() - b.eval(); }
		MathFunction derive(const MathVar &var) { return a.derive(var) - b.derive(var); }
		
		/**
		* Вернуть в виде строки
		*/
		std::string toString() {
			return a.toString() + " - " + b.toString();
		}
	};
	
	/**
	* Функция F(x,y) = x * y
	*/
	class MathMult: public MathFunctionImpl
	{
	private:
		MathFunction a;
		MathFunction b;
	public:
		MathMult(const MathFunction &A, const MathFunction &B): a(A), b(B) { }
		double eval() { return a.eval() * b.eval(); }
		MathFunction derive(const MathVar &var) { return a.derive(var) * b + a * b.derive(var); }
		
		/**
		* Вернуть в виде строки
		*/
		std::string toString() {
			return "(" + a.toString() + ") * (" + b.toString() + ")";
		}
	};
	
	/**
	* Функция F(x) = cos(x)
	*/
	class MathCos: public MathFunctionImpl
	{
	private:
		MathFunction a;
	public:
		MathCos(const MathFunction &A): a(A) { }
		double eval() { return ::cos(a.eval()); }
		MathFunction derive(const MathVar &var) { return - sin(a) * a.derive(var); }
		
		/**
		* Вернуть в виде строки
		*/
		std::string toString() {
			return "cos(" + a.toString() + ")";
		}
	};
	
	/**
	* Функция F(x) = sin(x)
	*/
	class MathSin: public MathFunctionImpl
	{
	private:
		MathFunction a;
	public:
		MathSin(const MathFunction &A): a(A) { }
		double eval() { return ::sin(a.eval()); }
		MathFunction derive(const MathVar &var) { return cos(a) * a.derive(var); }
		
		/**
		* Вернуть в виде строки
		*/
		std::string toString() {
			return "sin(" + a.toString() + ")";
		}
	};
	
	/**
	* Конструктор константной функции
	*/
	MathFunction::MathFunction(double c)
	{
		func = new MathConst(c);
		func->lock();
	}
	
	/**
	* Конструктор функции
	*/
	MathFunction::MathFunction(MathFunctionImpl *impl)
	{
		func = impl;
		if ( func ) func->lock();
	}
	
	/**
	* Конструктор копии
	*/
	MathFunction::MathFunction(const MathFunction &f)
	{
		func = f.func;
		if ( func ) func->lock();
	}
	
	/**
	* Деструктор функции
	*/
	MathFunction::~MathFunction()
	{
		if ( func ) func->release();
	}
	
	/**
	* Унарный оператор вычитания
	*/
	MathFunction operator - (const MathFunction &a)
	{
		return new MathNeg(a);
	}
	
	/**
	* Сумма функций
	*/
	MathFunction operator + (const MathFunction &a, const MathFunction &b)
	{
		return new MathSum(a, b);
	}
	
	/**
	* Сумма функции и константы
	*/
	MathFunction operator + (const MathFunction &a, double b)
	{
		return new MathSum(a, b);
	}
	
	/**
	* Сумма функции и константы
	*/
	MathFunction operator + (double a, const MathFunction &b)
	{
		return new MathSum(a, b);
	}
	
	/**
	* Разность функций
	*/
	MathFunction operator - (const MathFunction &a, const MathFunction &b)
	{
		return new MathSub(a, b);
	}
	
	/**
	* Сумма функции и константы
	*/
	MathFunction operator - (const MathFunction &a, double b)
	{
		return new MathSub(a, b);
	}
	
	/**
	* Сумма функции и константы
	*/
	MathFunction operator - (double a, const MathFunction &b)
	{
		return new MathSub(a, b);
	}
	
	/**
	* Произведение функций
	*/
	MathFunction operator * (const MathFunction &a, const MathFunction &b)
	{
		return new MathMult(a, b);
	}
	
	/**
	* Произведение функции и константы
	*/
	MathFunction operator * (const MathFunction &a, double b)
	{
		return new MathMult(a, b);
	}
	
	/**
	* Произведение функции и константы
	*/
	MathFunction operator * (double a, const MathFunction &b)
	{
		return new MathMult(a, b);
	}
	
	/**
	* Функция sin(x)
	*/
	MathFunction sin(const MathFunction &x)
	{
		return new MathSin(x);
	}
	
	/**
	* Функция cos(x)
	*/
	MathFunction cos(const MathFunction &x)
	{
		return new MathCos(x);
	}
}