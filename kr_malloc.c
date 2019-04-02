/*
kr c p185 mallo / free 的实现
@see https://stackoverflow.com/a/46291725
@see https://gnuchops.wordpress.com/2013/02/26/memory-allocator-for-embedded-system-k-r-ritchie-book/
*/

#include <stdio.h>
#include <unistd.h>

typedef long Align;

typedef union Header_u {
	struct {
		union Header_u *next;
		unsigned int size;
	} s;
	Align align;
} Header;

static Header sentinel;
static Header *freeList = NULL;

static Header *_kr_morecore(unsigned int bytes);
void free(void *p);

void *kr_malloc(unsigned int bytes) {
	Header *current, *previous; // current: 当前 block, previous: 上一个 block
	unsigned int nHeaders;

	// 计算需要几个 header 的大小，以 header 大小为单位分配空间，可以保证所有类型都 properly aligned
	nHeaders = (bytes + sizeof(Header) - 1) / sizeof(Header) + 1;
	if((previous = freeList) == NULL) { // freeList 为空，即第一次调用 kr_malloc
		sentinel.s.next = freeList = previous = &sentinel; // 这是一个假的自循环链表
		sentinel.s.size = 0;
	}

	// 遍历链表
	for (current = previous->s.next; ; previous = current, current = current->s.next) { 
		if (current->s.size >= nHeaders) { // 足够大
			if(current->s.size == nHeaders) { // 正好适合
				previous->s.next = current->s.next;
			} else {
				current->s.size -= nHeaders; // 减去即将分配出去的
				current += current->s.size; // 移动 current 指针
				current->s.size = nHeaders; // 设置大小
				// curent->s.next 将会在 kr_free 时重新分配
			}
			freeList = previous; // 记录本次分配的点位
			return (void *)(current + 1); // 返回可用的指针
		}

		if (current == freeList) { // 已经转了一圈
			if((current = _kr_morecore(nHeaders)) == NULL) { // 分配一块新内存
				return NULL; // 内存分配失败
			}
		}
	}
}

#define SBRK_THRESHOLD 1024 // 每次向系统请求的最少字节

static Header *_kr_morecore(unsigned int nHeaders) {
	char *result;
	Header *block;

	if(nHeaders < SBRK_THRESHOLD) {
		nHeaders = SBRK_THRESHOLD;
	}
	# error sbrk 已经废弃
	result = sbrk(nHeaders * sizeof(Header));
	if(result == (char *)-1) { // 为什么这样比较呢？
		return NULL;
	}
	block = (Header *)result;
	block->s.size = nHeaders;
	free((void *)(block + 1));
	return freeList;
}


void kr_free(void *ptr) {
	Header *block, *current;
	block = (Header *)ptr - 1;

	// 遍历链表，寻找插入位置
	for (current = freeList; !(block > current && block < currnet->s.next); current = current->s.next) { // block 在某个区间内
		if(current >= current->s.next && (block > current || block < current->s.next)) { // 此时 current 是最后一个（block 在头尾）
			break;
		}
	}

	// 先说后面
	if (block + block->s.size == current->s.next) { // 向后融为一体
		block->s.size += current->s.next->s.size;
		block.s->next = current->s.next->s.next;
	} else {
		block->s.next = current->s.next; // 在 current 和 current->s.next 中间插入
	}

	// 再说前面
	if (current + current->s.size == block) { // 向前融为一体
		current->s.size += block->s.size;
		current->s.next = block->s.next;
	} else {
		current->s.next = block;
	}
	freeList = current;
}

int main(int argc, char* argv[]) {
	size_t size = sizeof(Header);
	printf("Header 类型的 size 为 %zu\n", size);
	return 0;
}