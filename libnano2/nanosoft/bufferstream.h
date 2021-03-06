#ifndef NANOSOFT_BUFFERSTREAM_H
#define NANOSOFT_BUFFERSTREAM_H

#include <nanosoft/stream.h>
#include <nanosoft/blockspool.h>

/**
 * Буферизующий поток
 *
 * Класс BufferStream работает по принципу FIFO (First In - First Out),
 * все что записывается в поток через метод write() буферизуется во внутреннем
 * буфере и потом может быть прочитано методом read()
 *
 * Данный класс позволяет сохранить поступающие потоковые данные, чтобы
 * прочитать их позже. Данные записанные методом write() сохраняются только
 * до тех пор пока они не будут прочитаны с помощью метода read()
 */
class BufferStream: public stream
{
private:
	
	/**
	 * Сслыка на пул блоков
	 */
	BlocksPool *bp;
	
	/**
	 * Размер буферизованных данных (в байтах)
	 */
	size_t size;
	
	/**
	 * Смещение в первом блоке к началу актуальных данных
	 * (размер данных которые уже были прочитанны в первом блоке)
	 */
	size_t offset;
	
	/**
	 * Размер квоты (в байтах)
	 *
	 * Задает ограничение на рост буфера
	 */
	size_t quota;
	
	/**
	 * Указатель на первый блок данных
	 */
	nano_block_t *first;
	
	/**
	 * Указатель на последний блок данных
	 */
	nano_block_t *last;
	
public:
	
	/**
	 * Конструктор
	 *
	 * BufferStream использует пул блоков (класс BlocksStream) для буферизации
	 * данных. Конструктор по умолчанию будет использовать глобальный пул,
	 * который возвращает функция bp_pool(). Глобальный пул должен быть
	 * предварительно инициализирован функцией bp_init_pool()
	 */
	BufferStream();
	
	/**
	 * Конструктор
	 *
	 * BufferStream использует пул блоков (класс BlocksStream) для буферизации
	 * данных. Пул должен существовать всё время пока существует поток
	 * 
	 * @param pool ссылка на пул блоков
	 */
	BufferStream(BlocksPool *pool);
	
	/**
	 * Деструктор
	 */
	~BufferStream();
	
	/**
	* Вернуть квоту буфера
	*/
	size_t getQuota() const { return quota; }
	
	/**
	* Установить квоту буфера
	*
	* Квоту можно установить больше чем уже записанно данных, новые данные
	* писать будет нельзя пока буфер не опустошиться до новой квоты
	*/
	void getQuota(size_t q) { quota = q; }
	
	/**
	 * Вернуть размер данных хранимых в буфере
	 */
	size_t getDataSize() const { return size; }
	
	/**
	 * Прочитать данные из потока
	 */
	virtual size_t read(void *buf, size_t buf_len);
	
	/**
	 * Записать данные в поток
	 *
	 * Данный метод либо примет все данные целиком, либо (если нет места)
	 * не запишет ни одного байта
	 */
	virtual size_t write(const void *buf, size_t buf_len);
	
	/**
	 * Очистить данные хранящиеся в буфере
	 */
	void clear();
	
};

#endif // NANOSOFT_BUFFERSTREAM_H
