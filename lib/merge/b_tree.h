#ifndef B_TREE_H //include guard
#define B_TREE_H

#include <iostream>
#include <fstream>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "memory_map.h"

using namespace std;

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

typedef enum {
    COPY_ON_WRITE, 
    REAL_CMB, 
    FAKE_CMB, 
    DRAM
} MODE;

typedef enum {
    BLACK,
    RED
} COLOR;

typedef enum {
    I,
    D,
    U
} OPR_CODE;

ofstream mylog;

// ## Verification
int HIT = 0;

// B-Tree
template <typename T> class BTree;
template <typename T> class BTreeNode;
class removeList;
// CMB
class CMB;
class BKMap;
// Leaf Cache
class LEAF_CACHE;
class LRU_QUEUE;
class LRU_QUEUE_ENTRY;
class RBTree;
class RBTreeNode;
// Append operation
template <typename T> class APPEND;
class APPEND_ENTRY;
template <typename T> class VALUE_POOL;

template <typename T>
class BTree{
	int m;				// degree
    int min_num; 		// minimun number of node
	u_int64_t root_id;		// Pointer to root node
	int fd;
	int block_size;
	int block_cap;
    MODE mode;
		
	public:
	    CMB* cmb;
        LEAF_CACHE* leafCache;
        APPEND<T>* append_map;

		BTree(char* filename, int degree, MODE _mode, bool lfcache, bool append);
		~BTree();

		void reopen(int _fd, MODE _mode, bool lfcache, bool append);

		void stat();
        void memory_map_addr();

		void tree_read(int fd, BTree* tree);
		void tree_write(int fd, BTree* tree);

		void node_read(u_int64_t block_id, BTreeNode<T>* node);
		void node_write(u_int64_t block_id, BTreeNode<T>* node);

		u_int64_t get_free_block_id();
		void set_block_id(u_int64_t block_id, bool bit);
		void print_used_block_id();

		void display_tree();
        void inorder_traversal(char* filename);

		void insertion(u_int64_t _k, T _v);
        void search(u_int64_t _k, T* buf);
        void update(u_int64_t _k, T _v);
		void deletion(u_int64_t _k);	
};

template <typename T>
class BTreeNode{

	public:	
		int m;				// degree
		int min_num; 		// minimun number of node
		int num_key;		// the number of keys
		u_int64_t *key;			// keys array
		T *value; 		// value array
		u_int64_t *child_id;	// array of child pointers
		bool is_leaf;		// Is leaf or not
		u_int64_t node_id;
        u_int64_t parent_id;
			
		BTreeNode(int _m, bool _is_leaf, u_int64_t _node_id);
		~BTreeNode();

		void stat();

		void display_tree(BTree<T>* t, MODE mode, int level);
        void inorder_traversal(BTree<T>* t, ofstream &outFile);

		void search(BTree<T>* t, u_int64_t _k, T* buf, u_int64_t rbtree_id = 0);
        u_int64_t update(BTree<T>* t, u_int64_t _k, T _v, removeList** list, u_int64_t rbtree_id = 0);

		u_int64_t traverse_insert(BTree<T>* t, u_int64_t _k, T _v, removeList** list);
		u_int64_t direct_insert(BTree<T>* t, u_int64_t _k, T _v, removeList** list, u_int64_t node_id1 = 0, u_int64_t node_id2 = 0, u_int64_t rbtree_id = 0);
		u_int64_t split(BTree<T>* t, u_int64_t node_id, u_int64_t parent_id, removeList** list);

		u_int64_t traverse_delete(BTree<T>* t, u_int64_t _k, removeList** _list);
		u_int64_t direct_delete(BTree<T>* t, u_int64_t _k, removeList** _list, u_int64_t rbtree_id = 0);
		u_int64_t rebalance(BTree<T>* t, int idx, removeList** _list);
		u_int64_t get_pred(BTree<T>* t);
		u_int64_t get_succ(BTree<T>* t);	
};

class removeList{
	public:
		int id;
		removeList* next;
		removeList(int _id, removeList* _next);
};

class CMB{
	int fd;
    off_t bar_addr;
	void* map_base;
    off_t map_idx;
    void* cache_base;
    MODE mode;

	public:
		CMB(MODE _mode);
		~CMB();

        void* get_map_base();

        void cmb_memcpy(void* dest, void* src, size_t len);
        void remap(off_t offset);

		void read(void* buf, off_t offset, int size);
		void write(off_t offset, void* buf, int size);

        // BKMap
		u_int64_t get_new_node_id();
		u_int64_t get_block_id(u_int64_t node_id);
		void update_node_id(u_int64_t node_id, u_int64_t block_id);
        // LRU_QUEUE Cache
        u_int64_t get_lru_id(u_int64_t node_id);
        void update_lru_id(u_int64_t node_id, u_int64_t value);
        // Meta data
        u_int64_t get_num_key(u_int64_t node_id);
        void update_num_key(u_int64_t node_id, u_int64_t value);
        u_int64_t get_lower(u_int64_t node_id);
        void update_lower(u_int64_t node_id, u_int64_t value);
        u_int64_t get_upper(u_int64_t node_id);
        void update_upper(u_int64_t node_id, u_int64_t value);
};

class BKMap{
    public:
        u_int64_t block_id;
        u_int64_t lru_id;
        u_int64_t num_key;
        u_int64_t lower;
        u_int64_t upper;
};

class LEAF_CACHE{
    public:
        LRU_QUEUE* LRU;
        RBTree* RBTREE;
};

class LRU_QUEUE{
    u_int64_t Q_front_idx;
    u_int64_t Q_rear_idx;
    u_int64_t free_Stack_idx;
    u_int64_t free_idx;
    u_int64_t capacity;
    u_int64_t num_of_cache;

    public:
        LRU_QUEUE(CMB* cmb);

        void stat(CMB* cmb);

        bool full();

        u_int64_t get_free_idx(CMB* cmb);
        void free_push(CMB* cmb, u_int64_t idx);
        u_int64_t free_pop(CMB* cmb);

        void enqueue(CMB* cmb, RBTree* rbtree, u_int64_t rbtree_id);
        u_int64_t dequeue(CMB* cmb);
        u_int64_t remove(CMB* cmb, u_int64_t node_id);
        void read(CMB* cmb, u_int64_t idx, LRU_QUEUE_ENTRY* buf);

        u_int64_t search(CMB* cmb, u_int64_t key, LRU_QUEUE_ENTRY* buf);    
    };

class LRU_QUEUE_ENTRY{
    public:
        u_int64_t RBTREE_idx;
        u_int64_t lastQ;
        u_int64_t nextQ;
};

class RBTree{
    u_int64_t root_idx;
    u_int64_t free_Stack_idx;
    u_int64_t free_idx;
    u_int64_t capacity;
    u_int64_t num_of_cache;

    public:
        RBTree(CMB* cmb);

        u_int64_t insert(CMB* cmb, u_int64_t node_id);
        void insertFix(CMB* cmb, u_int64_t idx);
        void remove(CMB* cmb, u_int64_t rbtree_idx);
        void removeFix(CMB* cmb, u_int64_t idx, u_int64_t cur_parent_idx);
        u_int64_t search(CMB* cmb, u_int64_t key);

        void stat(CMB* cmb);
        void display(CMB* cmb);

        u_int64_t get_free_idx(CMB* cmb);
        void free_push(CMB* cmb, u_int64_t idx);
        u_int64_t free_pop(CMB* cmb);

        void readNode(CMB* cmb, u_int64_t idx, RBTreeNode* buf);
        void writeNode(CMB* cmb, u_int64_t idx, RBTreeNode* buf);

        void leafRotation(CMB* cmb, u_int64_t x_idx);
        void rightRotation(CMB* cmb, u_int64_t x_idx);

        u_int64_t succesor(CMB* cmb, u_int64_t idx);
};

class RBTreeNode{
    public:
        u_int64_t node_id;
        u_int64_t left_child_idx;
        u_int64_t right_child_idx;  
        u_int64_t color;    // Red - 1, Black - 0
        u_int64_t parent_idx;   

        RBTreeNode(u_int64_t _node_id, u_int64_t _color, u_int64_t _parent_idx);   

        void display(CMB* cmb, u_int64_t idx, int level);
};

template <typename T>
class APPEND{
    public:
        VALUE_POOL<T>* value_pool;

        void append_entry(BTree<T>* t, u_int64_t node_id, OPR_CODE OPR, u_int64_t _k, T _v);
        bool full(CMB* cmb, u_int64_t node_id);
        void reduction(BTree<T>* t, u_int64_t node_id, BTreeNode<T>* node);
        u_int64_t search_entry(CMB* cmb, u_int64_t node_id, u_int64_t key);

        u_int64_t get_num(CMB* cmb, u_int64_t node_id);
        void write_num(CMB* cmb, u_int64_t node_id, u_int64_t num);

        OPR_CODE get_opr(CMB* cmb, u_int64_t node_id, u_int64_t idx);
        u_int64_t get_key(CMB* cmb, u_int64_t node_id, u_int64_t idx);
        T get_value(CMB* cmb, u_int64_t node_id, u_int64_t idx);
        void delete_entries(CMB* cmb, u_int64_t node_id);
};

class APPEND_ENTRY{
    public:
        u_int64_t opr;
        u_int64_t key;
        u_int64_t value_idx;
};

template <typename T>
class VALUE_POOL{
    public:
        u_int64_t free_Stack_idx;
        u_int64_t free_idx;
        u_int64_t capacity;
        u_int64_t num;

        VALUE_POOL(CMB* cmb);

        void stat(CMB* cmb);

        u_int64_t get_free_idx(CMB* cmb);
        void free_push(CMB* cmb, u_int64_t idx);
        u_int64_t free_pop(CMB* cmb);

        void get_value(CMB* cmb, u_int64_t idx, T* buf);
        void write_value(CMB* cmb, u_int64_t idx, T* buf);
};

template <typename T>
BTree<T>::BTree(char* filename, int degree, MODE _mode, bool lfcache, bool append){
    mylog << "BTree()" << endl;

    if(degree > (int)((PAGE_SIZE - sizeof(BTreeNode<T>) - sizeof(u_int64_t)) / (sizeof(u_int64_t) + sizeof(T))) ){
        cout << " Error: Degree exceed " << endl;
        return;
    }

    fd = open(filename, O_RDWR | O_DIRECT);
    block_size = PAGE_SIZE;
    m = degree;
    min_num = (m % 2 == 0) ? m / 2 - 1 : m / 2;
    block_cap = (block_size - sizeof(BTree)) * 8;
    root_id = 0;
    mode = _mode;
    leafCache = NULL;
    append_map = NULL;
    
    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);
    
    memcpy(buf, this, sizeof(BTree));
    pwrite(fd, buf, PAGE_SIZE, 1 * PAGE_SIZE);

    free(buf);

    set_block_id(0, true); // block 0 is reserved
    set_block_id(1, true);

    if(mode == COPY_ON_WRITE)
        cmb = NULL;
    else{
        // Map CMB
        cmb = new CMB(mode);    
        u_int64_t first_node_id = 1;
        cmb->write(0, &first_node_id, sizeof(u_int64_t));

        // Interval to Leaf Cache Optimization
        if(lfcache){
            leafCache = new LEAF_CACHE();
            leafCache->LRU = new LRU_QUEUE(cmb);
            leafCache->LRU->stat(cmb);
            leafCache->RBTREE = new RBTree(cmb);
            leafCache->RBTREE->stat(cmb);
        }     

        // Append entry Optimization
        if(append){
            append_map = new APPEND<T>();
            append_map->value_pool = new VALUE_POOL<T>(cmb);
        }       
    }

}

template <typename T>
BTree<T>::~BTree(){
    mylog << "~BTree()" << endl;
    close(fd);
    if(cmb) delete cmb;
    if(leafCache){
        delete leafCache->LRU;
        delete leafCache->RBTREE;
        delete leafCache;
    } 
    if(append_map){
        delete append_map->value_pool;
        delete append_map;
    }
}

template <typename T>
void BTree<T>::reopen(int _fd, MODE _mode, bool lfcache, bool append){
    mylog << "reopen()" << endl;
    fd = _fd;
    mode = _mode;
    tree_write(fd, this);
    if(mode == COPY_ON_WRITE)
        cmb = NULL;
    else{
        cmb = new CMB(mode);
        if(lfcache){
            leafCache = new LEAF_CACHE();

            off_t addr;

            leafCache->LRU = (LRU_QUEUE*) malloc(sizeof(LRU_QUEUE));
            addr = LRU_QUEUE_START_ADDR;
            cmb->read(leafCache->LRU, addr, sizeof(LRU_QUEUE));

            leafCache->RBTREE = (RBTree*) malloc(sizeof(RBTree));
            addr = RED_BLACK_TREE_START_ADDR;
            cmb->read(leafCache->RBTREE, addr, sizeof(RBTree));
        }

        if(append){
            append_map = new APPEND<T>();
            append_map->value_pool = (VALUE_POOL<T>*) malloc(sizeof(VALUE_POOL<T>));
            off_t addr = VALUE_POOL_START_ADDR;
            cmb->read(append_map->value_pool, addr, sizeof(VALUE_POOL<T>));
        }
    }

}

template <typename T>
void BTree<T>::stat(){
    cout << endl;
    cout << "Degree: " << m << endl;
    cout << "fd: " << fd << endl;
    cout << "Block size: " << block_size << endl;
    cout << "Block Capacity: " << block_cap << endl;
    cout << "Root Block ID: " << root_id << endl;
    cout << endl;

    mylog << "BTree.stat()" << endl;
    mylog << "\tDegree: " << m << endl;
    mylog << "\tfd: " << fd << endl;
    mylog << "\tBlock size: " << block_size << endl;
    mylog << "\tBlock Capacity: " << block_cap << endl;
    mylog << "\tRoot Block ID: " << root_id << endl;
    print_used_block_id();
}

template <typename T>
void BTree<T>::memory_map_addr(){
    cout << "START_ADDR:\t\t\t0x" << hex << START_ADDR << endl;
    cout << "BLOCK_MAPPING_START_ADDR:\t0x" << hex << BLOCK_MAPPING_START_ADDR << endl;
    cout << "BLOCK_MAPPING_END_ADDR:\t\t0x" << hex << BLOCK_MAPPING_END_ADDR << endl;
    cout << "LRU_QUEUE_START_ADDR:\t\t0x" << hex << LRU_QUEUE_START_ADDR << endl;
    cout << "LRU_QUEUE_BASE_ADDR:\t\t0x" << hex << LRU_QUEUE_BASE_ADDR << endl;
    cout << "LRU_QUEUE_END_ADDR:\t\t0x" << hex << LRU_QUEUE_END_ADDR << endl;
    cout << "RED_BLACK_TREE_START_ADDR:\t0x" << hex << RED_BLACK_TREE_START_ADDR << endl;
    cout << "RED_BLACK_TREE_BASE_ADDR:\t0x" << hex << RED_BLACK_TREE_BASE_ADDR << endl;
    cout << "RED_BLACK_TREE_END_ADDR:\t0x" << hex << RED_BLACK_TREE_END_ADDR << endl;
    cout << "APPEND_OPR_START_ADDR:\t\t0x" << hex << APPEND_OPR_START_ADDR << endl;
    cout << "APPEND_OPR_END_ADDR:\t\t0x" << hex << APPEND_OPR_END_ADDR << endl;
    cout << "VALUE_POOL_START_ADDR:\t\t0x" << hex << VALUE_POOL_START_ADDR << endl;
    cout << "VALUE_POOL_BASE_ADDR:\t\t0x" << hex << VALUE_POOL_BASE_ADDR << endl;
    cout << "VALUE_POOL_END_ADDR:\t\t0x" << hex << VALUE_POOL_END_ADDR << endl; 
    cout << "END_ADDR:\t\t\t0x" << hex << END_ADDR << endl;
}

template <typename T>
void BTree<T>::tree_read(int fd, BTree* tree){ 
    mylog << "tree_read()" << endl;

    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);
    pread(fd, buf, PAGE_SIZE, 1 * PAGE_SIZE);
    
    memcpy((void*)tree, buf, sizeof(BTree));
    free(buf);
}

template <typename T>
void BTree<T>::tree_write(int fd, BTree* tree){
    mylog << "tree_write()" << endl;

    // Read before write to maintain the same block bitmap

    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);
    pread(fd, buf, PAGE_SIZE, 1 * PAGE_SIZE);
    
    memcpy(buf, tree, sizeof(BTree));
    pwrite(fd, buf, PAGE_SIZE, 1 * PAGE_SIZE);
    free(buf);
}

template <typename T>
void BTree<T>::node_read(u_int64_t node_id, BTreeNode<T>* node){
    mylog << "node_read() - node_id:" << node_id << endl;

    if(node_id == 0){
        cout << " error : attempt to read node id = 0 " << endl;
        mylog << " error : attempt to read node id = 0 " << endl;
        return;
    }

    delete [] node->key;
    delete [] node->value;
    delete [] node->child_id;

    u_int64_t block_id;
    if(cmb)
        block_id = cmb->get_block_id(node_id);       
    else
        block_id = node_id;

    if(block_id < 2){
        cout << " error : attempt to read block id = " << block_id << endl;
        mylog << " error : attempt to read block id = " << block_id << endl;
        return;
    }

    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);
    pread(fd, buf, PAGE_SIZE, block_id * PAGE_SIZE);

    char* ptr = buf;
    memcpy((void*)node, ptr, sizeof(BTreeNode<T>));
    ptr += sizeof(BTreeNode<T>);

    node->key = new u_int64_t[node->m];
    memcpy(node->key, ptr, node->m * sizeof(u_int64_t));
    ptr += node->m * sizeof(u_int64_t);

    node->value = new T[node->m];
    memcpy(node->value, ptr, node->m * sizeof(T));
    ptr += node->m * sizeof(T);

    node->child_id = new u_int64_t[node->m + 1];
    memcpy(node->child_id, ptr, (node->m + 1) * sizeof(u_int64_t));
    ptr += (node->m + 1) * sizeof(u_int64_t);

    free(buf);
}

template <typename T>
void BTree<T>::node_write(u_int64_t node_id, BTreeNode<T>* node){
    mylog << "node_write() - node_id:" << node_id << endl;

    if(node_id == 0){
        cout << " Error : Attempt to write node id = 0 " << endl;
        mylog << " Error : Attempt to write node id = 0 " << endl;
        return;
    }

    u_int64_t block_id;
    if(cmb)
        block_id = cmb->get_block_id(node_id);
    else
        block_id = node_id;

    if(block_id < 2){
        cout << " error : attempt to write block id = " << block_id << endl;
        mylog << " error : attempt to write block id = " << block_id << endl;
        return;
    }

    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);

    char* ptr = buf;
    memcpy(ptr, node, sizeof(BTreeNode<T>));
    ptr += sizeof(BTreeNode<T>);

    memcpy(ptr, node->key, node->m * sizeof(u_int64_t));
    ptr += node->m * sizeof(u_int64_t);

    memcpy(ptr, node->value, node->m * sizeof(T));
    ptr += node->m * sizeof(T);

    memcpy(ptr, node->child_id, (node->m + 1) * sizeof(u_int64_t));
    ptr += (node->m + 1) * sizeof(u_int64_t);

    pwrite(fd, buf, PAGE_SIZE, block_id * PAGE_SIZE);

    free(buf);
}

template <typename T>
u_int64_t BTree<T>::get_free_block_id(){
    mylog << "get_free_block_id()" << endl;

    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);
    pread(fd, buf, PAGE_SIZE, 1 * PAGE_SIZE);

    char* byte_ptr =  buf + sizeof(BTree);
    u_int64_t id = 0;
    while(id < block_cap){
        char byte = *byte_ptr;
        for(int i = 0; i < 8; i++){
            if( !(byte & 1) ){
                free(buf);
                set_block_id(id, true);
                return id;
            }
            byte >>= 1;
            id++;
        }
        byte_ptr++;
    }
    free(buf);
    return 0;
}

template <typename T>
void BTree<T>::set_block_id(u_int64_t block_id, bool bit){
    mylog << "set_block_id() - set block id:" << block_id << " as " << bit << endl;

    if(block_id >= block_cap) return;

    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);
    pread(fd, buf, PAGE_SIZE, 1 * PAGE_SIZE);

    char* byte_ptr =  buf + sizeof(BTree) + (block_id / 8);

    if(bit)
        *byte_ptr |= (1UL << (block_id % 8));
    else
        *byte_ptr &= ~(1UL << (block_id % 8));

    pwrite(fd, buf, PAGE_SIZE, 1 * PAGE_SIZE);

    free(buf);
}

template <typename T>
void BTree<T>::print_used_block_id(){
    mylog << "print_used_block_id()" << endl;
    mylog << "\tUsed Block : " << endl << "\t";

    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);
    pread(fd, buf, PAGE_SIZE, 1 * PAGE_SIZE);

    int num_bk = 0;
    char* byte_ptr =  buf + sizeof(BTree);
    int id = 0;
    while(id < block_cap){
        char byte = *byte_ptr;
        for(int i = 0; i < 8; i++){
            if( (byte & 1) ){
                mylog << " " << id;
                num_bk++;
            }
            byte >>= 1;
            id++;
        }
        byte_ptr++;
    }
    mylog << endl << "#Number of Used Block = " << num_bk << endl;

    free(buf);
}

template <typename T>
void BTree<T>::display_tree(){
    cout << "display_tree()" << endl;

    if(root_id){
        BTreeNode<T>* root = new BTreeNode<T>(0, 0, 0);
        node_read(root_id, root);
        root->display_tree(this, mode, 0);
        delete root;
    }
    else
        cout << " The tree is empty! " << endl;
}

template <typename T>
void BTree<T>::inorder_traversal(char* filename){
    mylog << endl << "inorder_traversal()" << endl;

    if(root_id){
        ofstream outFile(filename);

        BTreeNode<T>* root = new BTreeNode<T>(0, 0, 0);
        node_read(root_id, root);
        root->inorder_traversal(this, outFile);
        delete root;
    }
    else
        mylog << " The tree is empty! " << endl;
}

template <typename T>
void BTree<T>::search(u_int64_t _k, T* buf){
    mylog << "search() - key:" << _k << endl;

    if(root_id){
        // Interval to Leaf Optimization
        u_int64_t hit_idx;

        if(leafCache){
            u_int64_t hit_idx = leafCache->RBTREE->search(cmb, _k);

            if(hit_idx != 0){
                u_int64_t rbtree_idx = leafCache->LRU->remove(cmb, hit_idx);
                // Hit
                // Remove the cache from the queue, then Read the value
                // ##
                HIT++;
                BTreeNode<T>* leaf = new BTreeNode<T>(0, 0, 0);
                node_read(hit_idx, leaf);
                leaf->search(this, _k, buf, rbtree_idx);
                delete leaf;

                return;
            }
        }

        BTreeNode<T>* root = new BTreeNode<T>(0, 0, 0);
        node_read(root_id, root);
        root->search(this, _k, buf);
        delete root;
    }
    else
        buf = NULL;
}

template <typename T>
void BTree<T>::update(u_int64_t _k, T _v){
    mylog << "update() : update key:" << _k << endl;

    if(root_id){
        removeList* rmlist = NULL;

        if(leafCache){
            u_int64_t hit_idx = leafCache->RBTREE->search(cmb, _k);

            if(hit_idx != 0){
                u_int64_t rbtree_idx = leafCache->LRU->remove(cmb, hit_idx);
                // Hit
                // Remove the cache from the queue, then Update the value 
                // ##
                HIT++;
                BTreeNode<T>* leaf = new BTreeNode<T>(0, 0, 0);
                node_read(hit_idx, leaf);
                leaf->update(this, _k, _v, &rmlist, rbtree_idx);
                delete leaf;

                if(rmlist){
                    removeList* cur = rmlist;
                    removeList* next = NULL;
                    while(cur != NULL){
                        next = cur->next;
                        set_block_id(cur->id, false);
                        delete cur;
                        cur = next;
                    }
                }

                return;
            }
        }

        BTreeNode<T>* root = new BTreeNode<T>(0, 0, 0);
        node_read(root_id, root);
        int dup_node_id = root->update(this, _k, _v, &rmlist);

        if(dup_node_id == 0){
            delete root;
            return;
        }

        if(dup_node_id != root_id){
            root_id = dup_node_id;
            tree_write(fd, this);
        }

        if(rmlist){
            removeList* cur = rmlist;
            removeList* next = NULL;
            while(cur != NULL){
                next = cur->next;
                set_block_id(cur->id, false);
                delete cur;
                cur = next;
            }
        }
        delete root;
    }
}

template <typename T>
void BTree<T>::insertion(u_int64_t _k, T _v){
    mylog << "insertion() - key:" << _k << endl;

    if(root_id){
        removeList* rmlist = NULL;

        if(leafCache){
            u_int64_t hit_idx = leafCache->RBTREE->search(cmb, _k);

            if(hit_idx != 0){
                u_int64_t rbtree_idx = leafCache->LRU->remove(cmb, hit_idx);

                if(cmb->get_num_key(hit_idx) < m - 1){
                    //Hit and no split
                    // ##
                    HIT++;
                    BTreeNode<T>* leaf = new BTreeNode<T>(0, 0, 0);
                    node_read(hit_idx, leaf);
                    leaf->direct_insert(this, _k, _v, &rmlist, 0, 0, rbtree_idx); //## Hit, argument should include the rbtree_idx
                    delete leaf;

                    if(rmlist){
                        removeList* cur = rmlist;
                        removeList* next = NULL;
                        while(cur != NULL){
                            next = cur->next;
                            set_block_id(cur->id, false);
                            delete cur;
                            cur = next;
                        }
                    }

                    return;
                }
                else{
                    leafCache->RBTREE->remove(cmb, rbtree_idx);
                }
            }
        }

        BTreeNode<T>* root = new BTreeNode<T>(0, 0, 0);
        node_read(root_id, root);
        int dup_node_id = root->traverse_insert(this, _k, _v, &rmlist);

        if(dup_node_id == 0){
            delete root;
            return;
        }

        if(dup_node_id != root_id){
            root_id = dup_node_id;
            tree_write(fd, this);
        }       

        node_read(root_id, root);
        if(root->num_key >= m || (cmb && cmb->get_num_key(root_id) >= m)){  
            int new_root_id;     

            if(cmb){
                new_root_id = cmb->get_new_node_id();
                cmb->update_node_id(new_root_id, get_free_block_id());
                cmb->update_lru_id(new_root_id, 0);
                cmb->update_num_key(new_root_id, 0);
            }
            else
                new_root_id = get_free_block_id();

            BTreeNode<T>* new_root = new BTreeNode<T>(m, false, new_root_id);
            node_write(new_root_id, new_root);

            root_id = root->split(this, root_id, new_root_id, &rmlist);

            tree_write(fd, this);

            delete new_root;    
        }
        
        if(rmlist){
            removeList* cur = rmlist;
            removeList* next = NULL;
            while(cur != NULL){
                next = cur->next;
                set_block_id(cur->id, false);
                delete cur;
                cur = next;
            }
        }
                
        delete root;
    }
    else{
        if(cmb){
            root_id = cmb->get_new_node_id();
            cmb->update_node_id(root_id, get_free_block_id());
            cmb->update_lru_id(root_id, 0);
            cmb->update_num_key(root_id, 0);
            if(append_map){
                append_map->write_num(cmb, root_id, 0);
            }
        }
        else
            root_id = get_free_block_id();

        tree_write(fd, this);

        BTreeNode<T>* root = new BTreeNode<T>(m, true, root_id);
        node_write(root_id, root);
        delete root;
        insertion(_k, _v);
    }
}

template <typename T>
void BTree<T>::deletion(u_int64_t _k){
    mylog << "deletion() - key:" << _k << endl;

    if(root_id){
        removeList* rmlist = NULL;

        if(leafCache){
            u_int64_t hit_idx = leafCache->RBTREE->search(cmb, _k);

            if(hit_idx != 0){
                u_int64_t rbtree_idx = leafCache->LRU->remove(cmb, hit_idx);

                if(cmb->get_num_key(hit_idx) > min_num){
                    //Hit and no merge
                    // ##
                    HIT++;
                    BTreeNode<T>* leaf = new BTreeNode<T>(0, 0, 0);
                    node_read(hit_idx, leaf);
                    leaf->direct_delete(this, _k, &rmlist, rbtree_idx);
                    delete leaf;

                    if(rmlist){
                        removeList* cur = rmlist;
                        removeList* next = NULL;
                        while(cur != NULL){
                            next = cur->next;
                            set_block_id(cur->id, false);
                            delete cur;
                            cur = next;
                        }
                    }

                    return;
                }
                else{
                    leafCache->RBTREE->remove(cmb, rbtree_idx);
                }
            }
        }

        BTreeNode<T>* root = new BTreeNode<T>(0, 0, 0);
        node_read(root_id, root);
        int dup_node_id = root->traverse_delete(this, _k, &rmlist);

        if(dup_node_id == 0){
            delete root;
            return;
        }

        if(root_id != dup_node_id){
            root_id = dup_node_id;
            tree_write(fd, this);
        }            

        node_read(root_id, root);
        if(root->num_key == 0){
            if(cmb)
                rmlist = new removeList(cmb->get_block_id(root_id), rmlist);
            else
                rmlist = new removeList(root_id, rmlist);
            if(root->is_leaf){
                root_id = 0;             
            }
            else{
                root_id = root->child_id[0];
            }
            tree_write(fd, this);
        } 
        
        if(rmlist){
            removeList* cur = rmlist;
            removeList* next = NULL;
            while(cur != NULL){
                next = cur->next;
                set_block_id(cur->id, false);
                delete cur;
                cur = next;
            }
        }
        delete root;          
    }
    else
        return;
}

template <typename T>
BTreeNode<T>::BTreeNode(int _m, bool _is_leaf, u_int64_t _node_id){
    mylog << "BTreeNode()" << endl;

    m = _m;
    min_num = (_m % 2 == 0) ? _m / 2 - 1 : _m / 2;
    num_key = 0;
    key = new u_int64_t[m];
    value = new T[m];
    child_id = new u_int64_t[m+1];
    is_leaf = _is_leaf;
    node_id = _node_id;
    parent_id = 0;
}

template <typename T>
BTreeNode<T>::~BTreeNode(){
    mylog << "~BTreeNode()" << endl;

    delete [] key;
    delete [] value;
    delete [] child_id;
}

template <typename T>
void BTreeNode<T>::stat(){
    mylog << "BTreeNode.stat()" << endl;
    mylog << "\tDegree: " << m << endl;
    mylog << "\tMinimun Key: " << min_num << endl;
    mylog << "\t# keys: " << num_key << endl;
    mylog << "\tIs leaf? : " << is_leaf << endl;
    mylog << "\tNode ID: " << node_id << endl;
}

template <typename T>
void BTreeNode<T>::display_tree(BTree<T>* t, MODE mode, int level){
    
    for(int j = 0; j < level; j++) cout << '\t';
    if(mode == COPY_ON_WRITE)
        cout << '[' << node_id << ']' << endl;
    else
        cout << '[' << node_id << "=>" << t->cmb->get_block_id(node_id) << ']' << endl;

    BTreeNode<T>* node = new BTreeNode<T>(0,0,0);
    t->node_read(node_id, node);
    if(t->append_map && is_leaf)
        t->append_map->reduction(t, node_id, node);

    int i = 0;
    for(i = 0; i < num_key; i++){
        if(!is_leaf){
            BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
            t->node_read(child_id[i], child);
            child->display_tree(t, mode, level + 1);
            delete child;
        }
        for(int j = 0; j < level; j++) cout << '\t';
        cout << node->key[i] << endl;;
    }
    if(!is_leaf){
        BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
        t->node_read(child_id[i], child);
        child->display_tree(t, mode, level + 1);
        delete child;
    }
}

template <typename T>
void BTreeNode<T>::inorder_traversal(BTree<T>* t, ofstream &outFile){
    mylog << "inorder_traversal()" << endl;

    BTreeNode<T>* node = new BTreeNode<T>(0,0,0);
    t->node_read(node_id, node);

    if(t->append_map && is_leaf){
        t->append_map->reduction(t, node_id, node);
    }

    int i = 0;
    for(i = 0; i < node->num_key; i++){
        if(!is_leaf){
            BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
            t->node_read(node->child_id[i], child);
            child->inorder_traversal(t, outFile);
            delete child;
        }
        outFile << node->key[i] << '\t' << node->value[i].str << endl;;
    }
    if(!is_leaf){
        BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
        t->node_read(node->child_id[i], child);
        child->inorder_traversal(t, outFile);
        delete child;
    }

    delete node;
}

template <typename T>
void BTreeNode<T>::search(BTree<T>* t, u_int64_t _k, T* buf, u_int64_t rbtree_id){
    mylog << "search() - key:" << _k << endl;

    if(t->append_map && is_leaf){
        u_int64_t entry_idx = t->append_map->search_entry(t->cmb, node_id, _k);
        if(entry_idx != 0){
            T ret_val = t->append_map->get_value(t->cmb, node_id, entry_idx);
            memcpy(buf, &ret_val, sizeof(T));

            if(t->leafCache && is_leaf){
                if(t->leafCache->LRU->full()){
                    u_int64_t rm_rb_idx = t->leafCache->LRU->dequeue(t->cmb);
                    t->leafCache->RBTREE->remove(t->cmb, rm_rb_idx);
                }
                // Enqueue
                if(rbtree_id){
                    // red black tree node already exist (hit)
                    t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rbtree_id);
                }
                else{
                    // Create red black tree node and enqueue
                    u_int64_t new_rb_idx = t->leafCache->RBTREE->insert(t->cmb, node_id);
                    t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, new_rb_idx);
                }
            } 

            return;
        }
    }

    int i;
    for(i = 0; i < num_key; i++){
        if(_k == key[i]){
            // Key match
            memcpy(buf, &value[i], sizeof(T));

            // Add to the leaf interval cache
            if(t->leafCache && is_leaf){
                if(t->leafCache->LRU->full()){
                    u_int64_t rm_rb_idx = t->leafCache->LRU->dequeue(t->cmb);
                    t->leafCache->RBTREE->remove(t->cmb, rm_rb_idx);
                }
                // Enqueue
                if(rbtree_id){
                    // red black tree node already exist (hit)
                    t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rbtree_id);
                }
                else{
                    // Create red black tree node and enqueue
                    u_int64_t new_rb_idx = t->leafCache->RBTREE->insert(t->cmb, node_id);
                    t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, new_rb_idx);
                }
            }                

            return;
        }
        if(_k < key[i]){
            if(!is_leaf){
                BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
                t->node_read(child_id[i], child);
                child->search(t, _k, buf);
                delete child;
            }
            else
                buf = NULL;

            return;
        }
    }

    if(!is_leaf){
        BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
        t->node_read(child_id[i], child);
        child->search(t, _k, buf);
        delete child;
    }
    else
        buf = NULL;
}

template <typename T>
u_int64_t BTreeNode<T>::update(BTree<T>* t, u_int64_t _k, T _v, removeList** list, u_int64_t rbtree_id){
    mylog << "update() - key:" << _k << endl;

    if(t->append_map && is_leaf){
        t->append_map->append_entry(t, node_id, U, _k, _v);

        // Add to the leaf interval cache
        if(t->leafCache && is_leaf){
            if(t->leafCache->LRU->full()){
                u_int64_t rm_rb_idx = t->leafCache->LRU->dequeue(t->cmb);
                t->leafCache->RBTREE->remove(t->cmb, rm_rb_idx);
            }
            // Enqueue
            if(rbtree_id){
                // red black tree node already exist (hit)
                t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rbtree_id);
            }
            else{
                // Create red black tree node and enqueue
                u_int64_t new_rb_idx = t->leafCache->RBTREE->insert(t->cmb, node_id);
                t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, new_rb_idx);
            }
        }    
    }

    int i;
    for(i = 0; i < num_key; i++){
        // Key match
        if(_k == key[i]){
            value[i] = _v;
            if(t->cmb){
                *list = new removeList(t->cmb->get_block_id(node_id), *list);
                t->cmb->update_node_id(node_id, t->get_free_block_id());
            }
            else{
                *list = new removeList(node_id, *list);
                node_id = t->get_free_block_id();
            }                

            t->node_write(node_id, this);

            // Add to the leaf interval cache
            if(t->leafCache && is_leaf){
                if(t->leafCache->LRU->full()){
                    u_int64_t rm_rb_idx = t->leafCache->LRU->dequeue(t->cmb);
                    t->leafCache->RBTREE->remove(t->cmb, rm_rb_idx);
                }
                // Enqueue
                if(rbtree_id){
                    // red black tree node already exist (hit)
                    t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rbtree_id);
                }
                else{
                    // Create red black tree node and enqueue
                    u_int64_t new_rb_idx = t->leafCache->RBTREE->insert(t->cmb, node_id);
                    t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, new_rb_idx);
                }
            }                

            return node_id;
        }
        if(_k < key[i])
            break;        
    }

    if(is_leaf)
        return 0; // Not found
    else{
        BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
        t->node_read(child_id[i], child);
        int dup_child_id = child->update(t, _k, _v, list);

        if(dup_child_id == 0){
            delete child;
            return 0;
        }

        // Only copy-on-write would propagate
        if(dup_child_id != child_id[i]){
            child_id[i] = dup_child_id;
            *list = new removeList(node_id, *list);
            node_id = t->get_free_block_id();
            t->node_write(node_id, this);
        }

        delete child;

        return node_id;
    }
}

template <typename T>
u_int64_t BTreeNode<T>::traverse_insert(BTree<T>* t, u_int64_t _k, T _v, removeList** list){
    mylog << "traverse_insert() - key:" << _k << endl;
    
    if(is_leaf){
        return direct_insert(t, _k, _v, list);
    }        
    else{
        int i;
        for(i = 0; i < num_key; i++){
            if(_k == key[i])
                return 0;     
            if(_k < key[i])
                break;
        }
        
        BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
        t->node_read(child_id[i], child);
        int dup_child_id = child->traverse_insert(t, _k, _v, list);

        if(dup_child_id == 0){
            delete child;
            return 0;
        }

        // Only copy-on-write would propagate
        if(dup_child_id != child_id[i]){
            child_id[i] = dup_child_id;
            *list = new removeList(node_id, *list);
            node_id = t->get_free_block_id();
            t->node_write(node_id, this);
        }
                
        t->node_read(child_id[i], child);
        if(child->num_key >= m)
            node_id = split(t, child_id[i], node_id, list);
        if(t->append_map){           
            if(t->cmb->get_num_key(child_id[i]) >= m)
                node_id = split(t, child_id[i], node_id, list);
        }
            

        delete child;

        return node_id;
    }
}

template <typename T>
u_int64_t BTreeNode<T>::direct_insert(BTree<T>* t, u_int64_t _k, T _v, removeList** list, u_int64_t node_id1, u_int64_t node_id2, u_int64_t rbtree_id){
    /* Assume the list is not full */
    if(num_key >= m) return node_id;

    mylog << "direct_insert() - key:" << _k << endl;

    if(t->append_map && is_leaf){
        t->append_map->append_entry(t, node_id, I, _k, _v);

        if(t->cmb->get_num_key(node_id) == 0){
            t->cmb->update_lower(node_id, _k);
            t->cmb->update_upper(node_id, _k);
        }
        else{
            if(_k < t->cmb->get_lower(node_id)) t->cmb->update_lower(node_id, _k);
            if(_k > t->cmb->get_upper(node_id)) t->cmb->update_upper(node_id, _k);
        }
        u_int64_t num_k = t->cmb->get_num_key(node_id);
        t->cmb->update_num_key(node_id, num_k+1);
    }
    else{
        int idx;
        for(idx = 0; idx < num_key; idx++){
            if(_k == key[idx])
                return node_id;     // No two keys are the same
            else{
                if(_k < key[idx])
                    break;
            }
        }

        int i;
        for(i = num_key; i > idx; i--){
            key[i] = key[i-1];
            value[i] = value[i-1];
            if(!is_leaf) child_id[i+1] = child_id[i];
        }
        if(!is_leaf) child_id[i+1] = child_id[i];

        key[idx] = _k;
        value[idx] = _v;
        if(node_id1 != 0) child_id[idx] = node_id1;
        if(node_id2 != 0) child_id[idx+1] = node_id2;
        num_key++;

        if(t->cmb){
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(node_id, t->get_free_block_id());
            t->cmb->update_num_key(node_id, num_key);
            if(idx == 0) t->cmb->update_lower(node_id, key[0]);
            if(idx == num_key - 1) t->cmb->update_upper(node_id, key[num_key - 1]);
        }
        else{
            *list = new removeList(node_id, *list);
            node_id = t->get_free_block_id();
        }

        t->node_write(node_id, this);
    }

    if(t->leafCache && is_leaf && num_key <= m - 1 && t->cmb->get_num_key(node_id) <= m - 1){
        u_int64_t lru_id = t->cmb->get_lru_id(node_id);
        // If lru cache existed
        if(lru_id != 0){
            // Re-enqueue if it is existed
            u_int64_t rm_rb_idx = t->leafCache->LRU->remove(t->cmb, node_id);
            t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rm_rb_idx);
        }
        else{
            if(t->leafCache->LRU->full()){
                u_int64_t rm_rb_idx = t->leafCache->LRU->dequeue(t->cmb);
                t->leafCache->RBTREE->remove(t->cmb, rm_rb_idx);
            }
            // Enqueue
            if(rbtree_id){
                // red black tree node already exist (hit)
                t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rbtree_id);
            }
            else{
                // Create red black tree node and enqueue
                u_int64_t new_rb_idx = t->leafCache->RBTREE->insert(t->cmb, node_id);
                t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, new_rb_idx);
            }
        }
    }

    return node_id;
}

template <typename T>
u_int64_t BTreeNode<T>::split(BTree<T>*t, u_int64_t spt_node_id, u_int64_t parent_id, removeList** list){
    mylog << "split() - node id:" << spt_node_id << " parent node id:" << parent_id << endl;
    
    BTreeNode<T>* node = new BTreeNode<T>(0, 0, 0);
    t->node_read(spt_node_id, node);
    if(t->append_map){
        t->append_map->reduction(t, spt_node_id, node);
    }

    BTreeNode<T>* parent = new BTreeNode<T>(0, 0, 0);
    t->node_read(parent_id, parent);  

    int new_node_id;
    if(t->cmb){
        new_node_id = t->cmb->get_new_node_id();
        t->cmb->update_node_id(new_node_id, t->get_free_block_id());
        t->cmb->update_lru_id(new_node_id, 0);
    }
    else
        new_node_id = t->get_free_block_id();
    BTreeNode<T>* new_node = new BTreeNode<T>(m, node->is_leaf, new_node_id);

    int i, j;
    for( i = min_num+1, j = 0; i < node->num_key; i++, j++){
        new_node->key[j] = node->key[i];
        new_node->value[j] = node->value[i];
        if(!node->is_leaf)
            new_node->child_id[j] = node->child_id[i];
        new_node->num_key++;
    }
    if(!node->is_leaf)
        new_node->child_id[j] = node->child_id[i];

    if(t->cmb){
        *list = new removeList(t->cmb->get_block_id(node->node_id), *list);
        
        t->cmb->update_num_key(new_node_id, new_node->num_key);
        t->cmb->update_lower(new_node_id, new_node->key[0]);
        t->cmb->update_upper(new_node_id, new_node->key[new_node->num_key-1]);

        t->cmb->update_node_id(node->node_id, t->get_free_block_id());
        t->cmb->update_num_key(node->node_id, min_num);
        t->cmb->update_upper(node->node_id, node->key[min_num-1]);
        
        if(t->append_map){
            t->append_map->delete_entries(t->cmb, node->node_id);
            t->append_map->write_num(t->cmb, new_node_id, 0);
        }
    }
    else{
        *list = new removeList(node->node_id, *list);
        node->node_id = t->get_free_block_id();    
    }
        
    int dup_par_id = parent->direct_insert(t, node->key[min_num], node->value[min_num], list, node->node_id, new_node_id);
    node->num_key = min_num;

    node->parent_id = dup_par_id;
    new_node->parent_id = dup_par_id;

    t->node_write(node->node_id, node);
    t->node_write(new_node_id, new_node);

    // Update or Create a new cache for node
    if(t->leafCache && node->is_leaf && node->num_key <= m - 1){
        u_int64_t lru_id = t->cmb->get_lru_id(node->node_id);
        // If lru cache existed
        if(lru_id != 0){
            // Re-enqueue if it is existed
            u_int64_t rm_rb_idx = t->leafCache->LRU->remove(t->cmb, node->node_id);
            t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rm_rb_idx);
        }
        else{
            if(t->leafCache->LRU->full()){
                u_int64_t rm_rb_idx = t->leafCache->LRU->dequeue(t->cmb);
                t->leafCache->RBTREE->remove(t->cmb, rm_rb_idx);
            }
            // Create red black tree node and enqueue
            u_int64_t new_rb_idx = t->leafCache->RBTREE->insert(t->cmb, node->node_id);
            t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, new_rb_idx);
        }
    }

    // Update or Create a new cache for new node
    if(t->leafCache && new_node->is_leaf && new_node->num_key <= m - 1){
        if(t->leafCache->LRU->full()){
            u_int64_t rm_rb_idx = t->leafCache->LRU->dequeue(t->cmb);
            t->leafCache->RBTREE->remove(t->cmb, rm_rb_idx);
        }
        // Create red black tree node and enqueue
        u_int64_t new_rb_idx = t->leafCache->RBTREE->insert(t->cmb, new_node->node_id);
        t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, new_rb_idx);
    }

    delete node;
    delete parent;
    delete new_node;

    return dup_par_id;
}

template <typename T>
u_int64_t BTreeNode<T>::traverse_delete(BTree<T> *t, u_int64_t _k, removeList** list){
    mylog << "traverse_delete() - key:" << _k << endl;
    
    int i;
    bool found = false;
    for(i = 0; i < num_key; i++){
        if(_k <= key[i]){
            if(_k == key[i]) found = true;
            break;
        }
    }

    if(is_leaf){            
        if(found)
            return direct_delete(t, _k, list);
        else
            return 0;
    } 
    else{
        if(found){
            BTreeNode<T>* node = new BTreeNode<T>(0, 0, 0);
            t->node_read(child_id[i+1], node);

            int succ_id = node->get_succ(t);
            BTreeNode<T>* succ = new BTreeNode<T>(0, 0, 0);
            t->node_read(succ_id, succ);

            if(succ->num_key > min_num){
                mylog << "borrow_from_succesor() - node id:" << node->node_id << endl;
                // Borrow from succ
                key[i] = succ->key[0];
                value[i] = succ->value[0];
                // Delete the kv from succ
                child_id[i+1] = node->traverse_delete(t, key[i], list);

                if(t->cmb){
                    *list = new removeList(t->cmb->get_block_id(node_id), *list);
                    t->cmb->update_node_id(node_id, t->get_free_block_id());
                    if(i == 0) t->cmb->update_lower(node_id, key[0]);
                    if(i == num_key-1) t->cmb->update_upper(node_id, key[num_key-1]);
                }
                else{
                    *list = new removeList(node_id, *list);
                    node_id = t->get_free_block_id();
                }
                t->node_write(node_id, this);
                i = i + 1;
            }
            else{
                t->node_read(child_id[i], node);

                int pred_id = node->get_pred(t);
                BTreeNode<T>* pred = new BTreeNode<T>(0, 0, 0);  
                t->node_read(pred_id, pred);

                mylog << "borrow_from_predecesor() - node id:" << node->node_id << endl;
                // Borrow from pred
                key[i] = pred->key[pred->num_key - 1];
                value[i] = pred->value[pred->num_key - 1];
                // Delete the kv form pred
                child_id[i] = node->traverse_delete(t, key[i], list);

                if(t->cmb){
                    *list = new removeList(t->cmb->get_block_id(node_id), *list);
                    t->cmb->update_node_id(node_id, t->get_free_block_id());
                    if(i == 0) t->cmb->update_lower(node_id, key[0]);
                    if(i == num_key-1) t->cmb->update_upper(node_id, key[num_key-1]);
                }
                else{
                    *list = new removeList(node_id, *list);
                    node_id = t->get_free_block_id();
                }

                t->node_write(node_id, this);
                
                delete pred;
            }

            delete node;
            delete succ;

            return rebalance(t, i, list);
        }
        else{
            BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
            t->node_read(child_id[i], child);

            int dup_child_id = child->traverse_delete(t, _k, list);

            if(dup_child_id == 0){
                delete child;
                return 0;
            }

            // Only copy-on-write would propagate
            if(dup_child_id != child_id[i]){
                child_id[i] = dup_child_id;
                *list = new removeList(node_id, *list);
                node_id = t->get_free_block_id();
                t->node_write(node_id, this);
            }

            delete child;

            return rebalance(t, i, list);
        }
    }
}

template <typename T>
u_int64_t BTreeNode<T>::direct_delete(BTree<T>* t, u_int64_t _k, removeList** list, u_int64_t rbtree_id){
    mylog << "direct_delete() - key:" << _k << endl;
    
    if(t->append_map && is_leaf){
        T empty;
        t->append_map->append_entry(t, node_id, D, _k, empty);
        num_key--;
        t->cmb->update_num_key(node_id, num_key);
    }
    else{
        int i, idx;
        for(i = 0; i < num_key; i++){
            if(key[i] == _k) break;
        }
        idx = i;
        
        if(i == num_key){
            mylog << "~~Key not found~~" << endl;
            if(t->leafCache && rbtree_id != 0){
                t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rbtree_id);
            }
            return 0;
        } 

        if(i < num_key-1){
            for(; i < num_key-1; i++){
                key[i] = key[i+1];
                value[i] = value[i+1];
                if(!is_leaf) child_id[i] = child_id[i+1];
            }
            if(!is_leaf) child_id[i] = child_id[i+1];
        }
        num_key--;

        if(t->cmb){
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(node_id, t->get_free_block_id());
            t->cmb->update_num_key(node_id, num_key);
            if(idx == 0) t->cmb->update_lower(node_id, key[0]);
            if(idx == num_key) t->cmb->update_upper(node_id, key[num_key - 1]);
        }
        else{
            *list = new removeList(node_id, *list);
            node_id = t->get_free_block_id();
        }

        t->node_write(node_id, this);
    }

    if(t->leafCache && is_leaf && num_key >= min_num){
        u_int64_t lru_id = t->cmb->get_lru_id(node_id);
        // If lru cache existed
        if(lru_id != 0){
            // Re-enqueue if it is existed
            u_int64_t rm_rb_idx = t->leafCache->LRU->remove(t->cmb, node_id);
            t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rm_rb_idx);
        }
        else{
            if(t->leafCache->LRU->full()){
                u_int64_t rm_rb_idx = t->leafCache->LRU->dequeue(t->cmb);
                t->leafCache->RBTREE->remove(t->cmb, rm_rb_idx);
            }
            // Enqueue
            if(rbtree_id){
                // red black tree node already exist (hit)
                t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rbtree_id);
            }
            else{
                // Create red black tree node and enqueue
                u_int64_t new_rb_idx = t->leafCache->RBTREE->insert(t->cmb, node_id);
                t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, new_rb_idx);
            }
        }
    }

    return node_id;
}

template <typename T>
u_int64_t BTreeNode<T>::rebalance(BTree<T>* t, int idx, removeList** list){

    BTreeNode<T>* node = new BTreeNode<T>(0, 0, 0);
    t->node_read(child_id[idx], node);

    if(node->num_key >= min_num){
        delete node;
        return node_id;
    }

    mylog << "rebalance() - node id:" << child_id[idx] << endl;

    BTreeNode<T>* left = new BTreeNode<T>(0, 0, 0);
    if(idx - 1 >= 0)
        t->node_read(child_id[idx - 1], left);
    BTreeNode<T>* right = new BTreeNode<T>(0, 0, 0);
    if(idx + 1 <= num_key)
        t->node_read(child_id[idx + 1], right);
    int trans_node_id;

    if(idx - 1 >= 0 && left->num_key > min_num){
        mylog << "borrow_from_left_sibling() - node id:" << child_id[idx] << endl;
        //Borrowing from left
        t->node_read(child_id[idx-1], left);
        t->node_read(child_id[idx], right);
        trans_node_id = (left->is_leaf) ? 0 : left->child_id[left->num_key];
            // Insert to right
        child_id[idx] = right->direct_insert(t, key[idx-1], value[idx-1], list, trans_node_id);
            // Borrow from left
        key[idx-1] = left->key[left->num_key - 1];
        value[idx-1] = left->value[left->num_key - 1];
            // Delete from left
        child_id[idx-1] = left->direct_delete(t, key[idx-1], list);

        if(t->cmb){
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(node_id, t->get_free_block_id());
            t->cmb->update_num_key(node_id, num_key);
            t->cmb->update_lower(node_id, key[0]);
        }
        else{
            *list = new removeList(node_id, *list);
            node_id = t->get_free_block_id();
        }

        t->node_write(node_id, this);          
    }
    else if(idx + 1 <= num_key && right->num_key > min_num){
        mylog << "borrow_from_rigtt_sibling() - node id:" << child_id[idx] << endl;
        //Borrowing from right
        t->node_read(child_id[idx], left);
        t->node_read(child_id[idx+1], right);
        trans_node_id = (right->is_leaf) ? 0 : right->child_id[0];
            // Insert to left
        child_id[idx] = left->direct_insert(t, key[idx], value[idx], list, 0, trans_node_id);
            // Borrow from right
        key[idx] = right->key[0];
        value[idx] = right->value[0];
            // Delete from left
        child_id[idx+1] = right->direct_delete(t, key[idx], list);
        
        if(t->cmb){
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(node_id, t->get_free_block_id());
            t->cmb->update_num_key(node_id, num_key);
            t->cmb->update_upper(node_id, key[0]);
        }
        else{
            *list = new removeList(node_id, *list);
            node_id = t->get_free_block_id();
        }

        t->node_write(node_id, this);         
    }   
    else{
        mylog << "merge() - node id:" << child_id[idx] << endl;

        //Merge with right unless idx = num_key
        if(idx == num_key) idx -= 1;
        t->node_read(child_id[idx], left);
        t->node_read(child_id[idx+1], right);
        trans_node_id = (right->is_leaf) ? 0 : right->child_id[0];
            // Insert parent's kv to left
        left->key[left->num_key] = key[idx];
        left->value[left->num_key] = value[idx];
        if(!left->is_leaf)
            left->child_id[left->num_key+1] = right->child_id[0];
        left->num_key++;
            // Insert right to left
        for(int i = 0; i < right->num_key; i++){
            left->key[left->num_key] = right->key[i];
            left->value[left->num_key] = right->value[i];
            if(!right->is_leaf) 
                left->child_id[left->num_key+1] = right->child_id[i+1];
            left->num_key++;  
        }
            // Store left node
        if(t->cmb){
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(left->node_id, t->get_free_block_id());
            t->cmb->update_num_key(left->node_id, left->num_key);
            t->cmb->update_upper(left->node_id, left->key[left->num_key-1]);
        }
        else{
            *list = new removeList(left->node_id, *list);
            left->node_id = t->get_free_block_id();
        }
        t->node_write(left->node_id, left);
            // Delete the parent's kv
        node_id = direct_delete(t, key[idx], list);
        child_id[idx] = left->node_id;

        if(t->cmb){
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(node_id, t->get_free_block_id());
        }
        else{
            *list = new removeList(node_id, *list);
            node_id = t->get_free_block_id();
        }

        t->node_write(node_id, this);

        if(t->cmb)
            *list = new removeList(t->cmb->get_block_id(right->node_id), *list);
        else
            *list = new removeList(right->node_id, *list);

        // Remove the leaf cache of right if it is in the queue
        if(t->leafCache && right->is_leaf){
            u_int64_t lru_id = t->cmb->get_lru_id(right->node_id);
            // If lru cache existed
            if(lru_id != 0){
                // Re-enqueue if it is existed
                u_int64_t rm_rb_idx = t->leafCache->LRU->remove(t->cmb, right->node_id);
                t->leafCache->RBTREE->remove(t->cmb, rm_rb_idx);
            }
        }

        // Update or Create a new cache for left
        if(t->leafCache && left->is_leaf && left->num_key >= min_num){
            u_int64_t lru_id = t->cmb->get_lru_id(left->node_id);
            // If lru cache existed
            if(lru_id != 0){
                // Re-enqueue if it is existed
                u_int64_t rm_rb_idx = t->leafCache->LRU->remove(t->cmb, left->node_id);
                t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, rm_rb_idx);
            }
            else{
                if(t->leafCache->LRU->full()){
                    u_int64_t rm_rb_idx = t->leafCache->LRU->dequeue(t->cmb);
                    t->leafCache->RBTREE->remove(t->cmb, rm_rb_idx);
                }
                // Create red black tree node and enqueue
                u_int64_t new_rb_idx = t->leafCache->RBTREE->insert(t->cmb, left->node_id);
                t->leafCache->LRU->enqueue(t->cmb, t->leafCache->RBTREE, new_rb_idx);
            }
        }
    }

    delete node;
    delete left,
    delete right;

    return node_id;
}

template <typename T>
u_int64_t BTreeNode<T>::get_pred(BTree<T>* t){
    mylog << "get_pred() - node id:" << node_id << endl;

    if(is_leaf)
        return node_id;
    else{
        BTreeNode<T>* node = new BTreeNode<T>(0, 0, 0);
        t->node_read(child_id[num_key], node);
        int ret = node->get_pred(t);
        delete node;
        return ret;
    }        
}

template <typename T>
u_int64_t BTreeNode<T>::get_succ(BTree<T>* t){
    mylog << "get_succ() - node id:" << node_id << endl; 

    if(is_leaf)
        return node_id;
    else{
        BTreeNode<T>* node = new BTreeNode<T>(0, 0, 0);
        t->node_read(child_id[0], node);
        int ret = node->get_succ(t);
        delete node;
        return ret;
    }
}

removeList::removeList(int _id, removeList* _next){
    id = _id;
    next = _next;
}

CMB::CMB(MODE _mode){
    mylog << "CMB()" << endl;

    mode = _mode;

    if (mode == REAL_CMB){
        if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
        printf("\n/dev/mem opened.\n");
        bar_addr = CMB_ADDR;
    }
    else if (mode == FAKE_CMB || mode == DRAM){
        if ((fd = open("dup_cmb", O_RDWR | O_SYNC)) == -1) FATAL; 
        printf("\ndup_cmb opened.\n");
        bar_addr = 0;
    }
    
    map_idx = bar_addr & ~MAP_SIZE;

        /* Map one page */  
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map_idx);
    if (map_base == (void*)-1) FATAL;
    printf("Memory mapped at address %p.\n", map_base);

    if(mode == DRAM){
        cache_base = malloc(MAP_SIZE);

        cout << " Cache base = " << cache_base << "\n" << endl;
        mylog << " Cache base = " << cache_base << "\n" << endl;
        u_int64_t* dest = (u_int64_t*) cache_base;
        u_int64_t* src = (u_int64_t*) map_base;
        cout << " Cache CMB " << endl;
        int i = MAP_SIZE / sizeof(u_int64_t);
        while(i--){
            *dest++ = *src++;
            cout << "--" << (1 - ((double)i / (MAP_SIZE / sizeof(u_int64_t)))) * 100 << "%--\r";
        }
    }
    else
        cache_base = NULL;
}

CMB::~CMB(){
    mylog << "~CMB()" << endl;

    if(mode == DRAM){
        memcpy(map_base, cache_base, MAP_SIZE);    
        free(cache_base);
    }

    if (munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);
}

void* CMB::get_map_base(){
    return map_base;
}

void CMB::cmb_memcpy(void* dest, void* src, size_t len){

    u_int64_t* d = (u_int64_t*) dest;
    u_int64_t* s = (u_int64_t*) src;
    for(int i = 0; i < len / sizeof(u_int64_t); i++)
        *d++ = *s++;
}

void CMB::remap(off_t offset){
    if( ((bar_addr + offset) & ~MAP_MASK) != map_idx ){
        //mylog << "remap() - offset:" << offset << dec << endl;
        if(cache_base)
            memcpy(map_base, cache_base, MAP_SIZE);  

        /* Unmap the previous */
        if (munmap(map_base, MAP_SIZE) == -1) FATAL;        

        map_idx = (bar_addr + offset) & ~MAP_MASK;

        /* Remap a new page */
        map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map_idx);
	    if (map_base == (void*)-1) FATAL;

        if(cache_base)
            cmb_memcpy(cache_base, map_base, MAP_SIZE);
    }
}

void CMB::read(void* buf, off_t offset, int size){
    //mylog << "CMB.read() - offset:" << hex << offset << dec  << endl;

    int total_size = size;
    int cur_size = size;
    off_t cur_offset = offset;
    char* cur_buf = (char*) buf + size;

    while(total_size){
        cur_offset = offset;
        cur_size = total_size;

        if(((bar_addr + offset) & ~MAP_MASK) < ((bar_addr + offset + total_size - 1) & ~MAP_MASK)){
            cur_offset = (offset + total_size - 1) & ~MAP_MASK;
            cur_size = (offset + total_size) - cur_offset;
        }
        cur_buf -= cur_size;

        remap(cur_offset);

        void* virt_addr;
        if(cache_base)
            virt_addr = (char*)cache_base + ((cur_offset) & MAP_MASK);	
        else
            virt_addr = (char*)map_base + ((cur_offset) & MAP_MASK);
        cmb_memcpy(cur_buf, virt_addr, cur_size);

        total_size -= cur_size;
    }
}

void CMB::write(off_t offset, void* buf, int size){
    mylog << "CMB.write() - offset:" << hex << offset << dec << ", size = " << size << endl;

    int total_size = size;
    int cur_size = size;
    off_t cur_offset = offset;
    char* cur_buf = (char*) buf + size;

    while(total_size){
        cur_offset = offset;
        cur_size = total_size;

        if(((bar_addr + offset) & ~MAP_MASK) < ((bar_addr + offset + total_size - 1) & ~MAP_MASK)){
            cur_offset = (offset + total_size - 1) & ~MAP_MASK;
            cur_size = (offset + total_size) - cur_offset;
        }
        cur_buf -= cur_size;

        remap(cur_offset);

        void* virt_addr;
        if(cache_base)
            virt_addr = (char*)cache_base + ((cur_offset) & MAP_MASK);	
        else
            virt_addr = (char*)map_base + ((cur_offset) & MAP_MASK);
        cmb_memcpy(virt_addr, cur_buf, cur_size);

        total_size -= cur_size;
    }
}

u_int64_t CMB::get_new_node_id(){
    mylog << "get_new_node_id()" << endl;

    u_int64_t new_node_id;
    read(&new_node_id, BLOCK_MAPPING_START_ADDR, sizeof(u_int64_t));

    u_int64_t next_node_id = new_node_id + 1;
    if(next_node_id > (BLOCK_MAPPING_END_ADDR - BLOCK_MAPPING_START_ADDR) / sizeof(u_int64_t)){
        cout << "CMB Block Mapping Limit Exceeded" << endl;
        mylog << "CMB Block Mapping Limit Exceeded" << endl;
        exit(1);
    }
    write(BLOCK_MAPPING_START_ADDR, &next_node_id, sizeof(u_int64_t));

    return new_node_id;
}

u_int64_t CMB::get_block_id(u_int64_t node_id){
    mylog << "get_block_id() - node id:" << node_id << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.block_id - (char*)&ref);
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Read Out of Range" << endl;
        mylog << "CMB Block Mapping Read Out of Range" << endl;
        exit(1);
    }

    u_int64_t readval;
	read(&readval, addr, sizeof(u_int64_t));
    return readval;
}

void CMB::update_node_id(u_int64_t node_id, u_int64_t block_id){
    mylog << "update_node_id() - node id:" << node_id << " block id:" << block_id << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.block_id - (char*)&ref);
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Write Out of Range" << endl;
        mylog << "CMB Block Mapping Write Out of Range" << endl;
        exit(1);
    }

    u_int64_t writeval = block_id;
	write(addr, &writeval, sizeof(u_int64_t));
}

u_int64_t CMB::get_lru_id(u_int64_t node_id){
    mylog << "get_lru_id() - node id:" << node_id << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.lru_id - (char*)&ref);
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Read Out of Range" << endl;
        mylog << "CMB Block Mapping Read Out of Range" << endl;
        exit(1);
    }

    u_int64_t readval;
	read(&readval, addr, sizeof(u_int64_t));
    return readval;
}

void CMB::update_lru_id(u_int64_t node_id, u_int64_t value){
    mylog << "update_lru_id() - node id:" << node_id << " value:" << value << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.lru_id - (char*)&ref);
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Write Out of Range" << endl;
        mylog << "CMB Block Mapping Write Out of Range" << endl;
        exit(1);
    }

	write(addr, &value, sizeof(u_int64_t));
}

u_int64_t CMB::get_num_key(u_int64_t node_id){
    mylog << "get_num_key() - node id:" << node_id << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.num_key - (char*)&ref);
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Read Out of Range" << endl;
        mylog << "CMB Block Mapping Read Out of Range" << endl;
        exit(1);
    }

    u_int64_t readval;
	read(&readval, addr, sizeof(u_int64_t));
    return readval;
}

void CMB::update_num_key(u_int64_t node_id, u_int64_t value){
    mylog << "update_num_key() - node id:" << node_id << " value:" << value << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.num_key - (char*)&ref);
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Write Out of Range" << endl;
        mylog << "CMB Block Mapping Write Out of Range" << endl;
        exit(1);
    }

	write(addr, &value, sizeof(u_int64_t));
}

u_int64_t CMB::get_lower(u_int64_t node_id){
    mylog << "get_lower() - node id:" << node_id << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.lower - (char*)&ref);
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Read Out of Range" << endl;
        mylog << "CMB Block Mapping Read Out of Range" << endl;
        exit(1);
    }

    u_int64_t readval;
	read(&readval, addr, sizeof(u_int64_t));
    return readval;
}

void CMB::update_lower(u_int64_t node_id, u_int64_t value){
    mylog << "update_lower() - node id:" << node_id << " value:" << value << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.lower - (char*)&ref);
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Write Out of Range" << endl;
        mylog << "CMB Block Mapping Write Out of Range" << endl;
        exit(1);
    }

	write(addr, &value, sizeof(u_int64_t));
}

u_int64_t CMB::get_upper(u_int64_t node_id){
    mylog << "get_upper() - node id:" << node_id << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.upper - (char*)&ref);
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Read Out of Range" << endl;
        mylog << "CMB Block Mapping Read Out of Range" << endl;
        exit(1);
    }

    u_int64_t readval;
	read(&readval, addr, sizeof(u_int64_t));
    return readval;
}

void CMB::update_upper(u_int64_t node_id, u_int64_t value){
    mylog << "update_upper() - node id:" << node_id << " value:" << value << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.upper - (char*)&ref);
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Write Out of Range" << endl;
        mylog << "CMB Block Mapping Write Out of Range" << endl;
        exit(1);
    }

	write(addr, &value, sizeof(u_int64_t));
}

LRU_QUEUE::LRU_QUEUE(CMB* cmb){
    mylog << "LRU_QUEUE()" << endl;
    Q_front_idx = 0;
    Q_rear_idx = 0;
    free_Stack_idx = 0;
    free_idx = 1;
    capacity = ((LRU_QUEUE_END_ADDR - LRU_QUEUE_BASE_ADDR) / sizeof(LRU_QUEUE_ENTRY)) - 1;
    num_of_cache = 0;

    off_t addr = LRU_QUEUE_START_ADDR;

    cmb->write(addr, this, sizeof(LRU_QUEUE));
}

void LRU_QUEUE::stat(CMB* cmb){
    LRU_QUEUE* meta = (LRU_QUEUE*) malloc(sizeof(LRU_QUEUE));

    off_t addr = LRU_QUEUE_START_ADDR;
    cmb->read(meta, addr, sizeof(LRU_QUEUE));

    cout << "LRU_QUEUE: stat()" << endl;
    cout << "Q_front_idx = " << meta->Q_front_idx << endl;
    cout << "Q_rear_idx = " << meta->Q_rear_idx << endl;
    cout << "free_Stack_idx = " << meta->free_Stack_idx << endl;
    cout << "free_idx = " << meta->free_idx << endl;
    cout << "capacity = " << meta->capacity << endl;
    cout << "num_of_cache = " << meta->num_of_cache << endl;

    u_int64_t cur_idx = meta->Q_front_idx;
    while(cur_idx){
        off_t addr = LRU_QUEUE_BASE_ADDR + cur_idx * sizeof(LRU_QUEUE_ENTRY);
        LRU_QUEUE_ENTRY* entry = new LRU_QUEUE_ENTRY();
        read(cmb, cur_idx, entry);

        RBTreeNode* treeNode = new RBTreeNode(0,0,0);
        addr = RED_BLACK_TREE_BASE_ADDR + entry->RBTREE_idx * sizeof(RBTreeNode);
        cmb->read(treeNode, addr, sizeof(RBTreeNode));

        cout << "< " << cur_idx << "->" << entry->RBTREE_idx << ". " << treeNode->node_id << ": " << cmb->get_lower(treeNode->node_id) << "-" << cmb->get_upper(treeNode->node_id) << " > - ";

        cur_idx = entry->nextQ;
        delete entry;
        delete treeNode;
    }
    cout << endl;

    free(meta);
}

bool LRU_QUEUE::full(){
    mylog << "LRU_QUEUE::full(): " << num_of_cache << "/" << capacity << endl;
    if(num_of_cache >= capacity)
        return true;
    return false;
}

u_int64_t LRU_QUEUE::get_free_idx(CMB* cmb){
    mylog << "LRU_QUEUE::get_free_idx()" << endl;
    // If the stack is empty
    if(free_Stack_idx == 0){
        off_t ret_idx = free_idx;
        free_idx++;

        // Write the new free_idx back
        off_t addr = LRU_QUEUE_START_ADDR + ((char*) &free_idx - (char*) this);
        cmb->write(addr, &free_idx, sizeof(u_int64_t));

        return ret_idx;
    }
    else
        return free_pop(cmb);
}

void LRU_QUEUE::free_push(CMB* cmb, u_int64_t idx){
    mylog << "LRU_QUEUE::free_push(): idx = " << idx << endl;
    // Set the new head->next as the current head idx
    off_t addr = LRU_QUEUE_BASE_ADDR + idx * sizeof(LRU_QUEUE_ENTRY);
    if(addr >= LRU_QUEUE_END_ADDR){
        cout << "Leaf Cache Limit Exceed" << endl;
        mylog << "Leaf Cache Limit Exceed" << endl;
        exit(1);
    }
    cmb->write(addr, &free_Stack_idx, sizeof(u_int64_t));

    // Set the stack head to the new head
    addr = LRU_QUEUE_START_ADDR + ((char*)&free_Stack_idx - (char*)this);
    cmb->write(addr, &idx, sizeof(u_int64_t)); 

    free_Stack_idx = idx;
}

u_int64_t LRU_QUEUE::free_pop(CMB* cmb){
    mylog << "LRU_QUEUE::free_pop()" << endl;
    u_int64_t ret, next;

    if(free_Stack_idx == 0)
        return 0;

    // Return value would be the stack head
    ret = free_Stack_idx;
    
    // Read the new stack head from the head idx
    off_t addr = LRU_QUEUE_BASE_ADDR + free_Stack_idx * sizeof(LRU_QUEUE_ENTRY);
    if(addr >= LRU_QUEUE_END_ADDR){
        cout << "Leaf Cache Limit Excced" << endl;
        mylog << "Leaf Cache Limit Excced" << endl;
        exit(1);
    }
    cmb->read(&next, addr, sizeof(u_int64_t));

    // Write the new stack head back
    free_Stack_idx = next;

    addr = LRU_QUEUE_START_ADDR + ((char*)&free_Stack_idx - (char*)this);
    cmb->write(addr, &free_Stack_idx, sizeof(u_int64_t));

    return ret;
}

void LRU_QUEUE::enqueue(CMB* cmb, RBTree* rbtree, u_int64_t rbtree_id){
    mylog << "LRU::enqueue() : " << "rbtree_id = " << rbtree_id << endl;
    if(full()){
        u_int64_t rm_rb_idx = dequeue(cmb);
        rbtree->remove(cmb, rm_rb_idx);
    }
    
    // Get the new idx
    u_int64_t new_idx = get_free_idx(cmb);

    // Write the cache to the CMB
    LRU_QUEUE_ENTRY* new_cache = new LRU_QUEUE_ENTRY();
    
    new_cache->RBTREE_idx = rbtree_id;
    new_cache->lastQ = Q_rear_idx;
    new_cache->nextQ = 0;

    off_t addr = LRU_QUEUE_BASE_ADDR + new_idx * sizeof(LRU_QUEUE_ENTRY);
    if(addr >= LRU_QUEUE_END_ADDR){
        cout << "Leaf Cache Limit Excced" << endl;
        mylog << "Leaf Cache Limit Excced" << endl;
        exit(1);
    }
    cmb->write(addr, new_cache, sizeof(LRU_QUEUE_ENTRY));

    // Set the old rear->next to new cache
    if(Q_rear_idx != 0){
        addr = LRU_QUEUE_BASE_ADDR + Q_rear_idx * sizeof(LRU_QUEUE_ENTRY) + ((char*)&(new_cache->nextQ) - (char*)new_cache);
        if(addr >= LRU_QUEUE_END_ADDR){
            cout << "Leaf Cache Limit Excced" << endl;
            mylog << "Leaf Cache Limit Excced" << endl;
            exit(1);
        }
        cmb->write(addr, &new_idx, sizeof(u_int64_t));
    }

    // Set the rear idx to the new cache idx
    // If front == 0 which means the queue is empty, set the front as well
    addr = LRU_QUEUE_START_ADDR + ((char*)&Q_rear_idx - (char*)this);
    if(addr >= LRU_QUEUE_END_ADDR){
        cout << "Leaf Cache Limit Excced" << endl;
        mylog << "Leaf Cache Limit Excced" << endl;
        exit(1);
    }
    cmb->write(addr, &new_idx, sizeof(u_int64_t));
    Q_rear_idx = new_idx;

    if(Q_front_idx == 0){
        addr = LRU_QUEUE_START_ADDR + ((char*)&Q_front_idx - (char*)this);
        if(addr >= LRU_QUEUE_END_ADDR){
            cout << "Leaf Cache Limit Excced" << endl;
            mylog << "Leaf Cache Limit Excced" << endl;
            exit(1);
        }
        cmb->write(addr, &new_idx, sizeof(u_int64_t));
        Q_front_idx = new_idx;
    }

    // Set the BKMap->lru_id
    u_int64_t node_id;
    RBTreeNode* rbtree_ref = new RBTreeNode(0,0,0);
    addr = RED_BLACK_TREE_BASE_ADDR + rbtree_id * sizeof(RBTreeNode) + ((char*)&rbtree_ref->node_id - (char*)rbtree_ref);
    cmb->read(&node_id, addr, sizeof(u_int64_t));
    delete rbtree_ref;
    cmb->update_lru_id(node_id, new_idx);

    num_of_cache++;
    addr = LRU_QUEUE_START_ADDR + ((char*) &num_of_cache - (char*) this);
    cmb->write(addr, &num_of_cache, sizeof(u_int64_t));

    delete new_cache;
}

u_int64_t LRU_QUEUE::dequeue(CMB* cmb){
    mylog << "LRU::dequeue()" << endl;
    if(Q_front_idx == 0)
        return 0;

    u_int64_t del_idx = Q_front_idx;

    // Read the second idx
    LRU_QUEUE_ENTRY* buf = new LRU_QUEUE_ENTRY();
    off_t addr = LRU_QUEUE_BASE_ADDR + Q_front_idx * sizeof(LRU_QUEUE_ENTRY);
    cmb->read(buf, addr, sizeof(LRU_QUEUE_ENTRY));

    u_int64_t rbtree_idx = buf->RBTREE_idx;

    // Set the front to the second idx
    addr = LRU_QUEUE_START_ADDR + ((char*) &Q_front_idx - (char*)this);
    cmb->write(addr, &(buf->nextQ), sizeof(u_int64_t));
    Q_front_idx = buf->nextQ;

    // If the Q_front_idx == 0, which means the queue is empty, set the rear to 0 as well
    if(Q_front_idx == 0){
        addr = LRU_QUEUE_START_ADDR + ((char*) &Q_rear_idx - (char*)this);
        cmb->write(addr, &Q_front_idx, sizeof(u_int64_t));
        Q_rear_idx = 0;
    }
    else{
        // Set front->lastQ to 0
        u_int64_t zero = 0;
        addr = LRU_QUEUE_BASE_ADDR + Q_front_idx * sizeof(LRU_QUEUE_ENTRY) + ((char*)&(buf->lastQ) - (char*)buf);
        cmb->write(addr, &zero, sizeof(u_int64_t));
    }

    // Clear the BKMap->lru_id
    u_int64_t node_id;
    RBTreeNode* rbtree_ref = new RBTreeNode(0,0,0);
    addr = RED_BLACK_TREE_BASE_ADDR + rbtree_idx * sizeof(RBTreeNode) + ((char*)&rbtree_ref->node_id - (char*)rbtree_ref);
    cmb->read(&node_id, addr, sizeof(u_int64_t));
    delete rbtree_ref;
    cmb->update_lru_id(node_id, 0);

    // return the free cache slot to free stack
    free_push(cmb, del_idx);

    num_of_cache--;
    addr = LRU_QUEUE_START_ADDR + ((char*) &num_of_cache - (char*) this);
    cmb->write(addr, &num_of_cache, sizeof(u_int64_t));

    delete buf;
    return rbtree_idx;
}

u_int64_t LRU_QUEUE::remove(CMB* cmb, u_int64_t node_id){
    mylog << "LRU::remove(): node_id = " << node_id << endl;
    if(node_id == 0) return 0;

    off_t addr;
    u_int64_t zero = 0;

    u_int64_t idx = cmb->get_lru_id(node_id);

    LRU_QUEUE_ENTRY* buf = new LRU_QUEUE_ENTRY();
    read(cmb, idx, buf);
    u_int64_t rbtree_id = buf->RBTREE_idx;

    if(Q_front_idx != idx){
        // Set the last->next = cur->next
        addr = LRU_QUEUE_BASE_ADDR + buf->lastQ * sizeof(LRU_QUEUE_ENTRY) + ((char*)&(buf->nextQ) - (char*)buf);
        cmb->write(addr, &(buf->nextQ), sizeof(u_int64_t));
    }
    else{
        // Set the front = cur->next
        addr = LRU_QUEUE_START_ADDR + ((char*)&Q_front_idx - (char*)this);
        cmb->write(addr, &(buf->nextQ), sizeof(u_int64_t));
        Q_front_idx = buf->nextQ;
    }

    if(Q_rear_idx != idx){
        // Set the next->last = cur->last
        addr = LRU_QUEUE_BASE_ADDR + buf->nextQ * sizeof(LRU_QUEUE_ENTRY) + ((char*)&(buf->lastQ) - (char*)buf);
        cmb->write(addr, &(buf->lastQ), sizeof(u_int64_t));
    }
    else{
        // Set the rear = cur->last
        addr = LRU_QUEUE_START_ADDR + ((char*)&Q_rear_idx - (char*)this);
        cmb->write(addr, &(buf->lastQ), sizeof(u_int64_t));
        Q_rear_idx = buf->lastQ;
    }

    // Clear the BKMap->lru_id
    cmb->update_lru_id(node_id, 0);

    free_push(cmb, idx);

    num_of_cache--;
    addr = LRU_QUEUE_START_ADDR + ((char*) &num_of_cache - (char*) this);
    cmb->write(addr, &num_of_cache, sizeof(u_int64_t));

    delete buf;

    return rbtree_id;
}

void LRU_QUEUE::read(CMB* cmb, u_int64_t idx, LRU_QUEUE_ENTRY* buf){
    mylog << "LRU_QUEUE::read(): idx = " << idx << endl;
    off_t addr = LRU_QUEUE_BASE_ADDR + idx * sizeof(LRU_QUEUE_ENTRY);
    if(addr >= LRU_QUEUE_END_ADDR){
        cout << "Leaf Cache Limit Excced" << endl;
        mylog << "Leaf Cache Limit Excced" << endl;
        exit(1);
    }
    cmb->read(buf, addr, sizeof(LRU_QUEUE_ENTRY));
}

u_int64_t LRU_QUEUE::search(CMB* cmb, u_int64_t key, LRU_QUEUE_ENTRY* buf){
    mylog << "LRU_QUEUE::search(): idx = " << key << endl;
    off_t addr;

    u_int64_t cur_idx = Q_front_idx;
    while(cur_idx != 0){
        u_int64_t rbtree_id, node_id;
        u_int64_t lower, upper;
        addr = LRU_QUEUE_BASE_ADDR + cur_idx * sizeof(LRU_QUEUE_ENTRY) + ((char*)&buf->RBTREE_idx - (char*)buf);
        if(addr >= LRU_QUEUE_END_ADDR){
            cout << "Leaf Cache Limit Excced" << endl;
            mylog << "Leaf Cache Limit Excced" << endl;
            exit(1);
        }
        cmb->read(&rbtree_id, addr, sizeof(u_int64_t));
        
        RBTreeNode* ref = new RBTreeNode(0,0,0);
        addr = RED_BLACK_TREE_BASE_ADDR + rbtree_id * sizeof(RBTreeNode) + ((char*)&ref->node_id - (char*)ref);
        cmb->read(&node_id, addr, sizeof(u_int64_t));

        if(key >= cmb->get_lower(node_id) && key <= cmb->get_upper(node_id))
            return cur_idx;
        cur_idx = buf->nextQ;
    }

    memset(buf, 0, sizeof(LRU_QUEUE_ENTRY));
    return 0;
}

RBTree::RBTree(CMB* cmb){
    root_idx = 0;
    free_idx = 1;
    capacity = ((RED_BLACK_TREE_END_ADDR - RED_BLACK_TREE_BASE_ADDR) / sizeof(RBTreeNode)) - 1;
    num_of_cache = 0;
    
    off_t addr = RED_BLACK_TREE_START_ADDR;
    cmb->write(addr, this, sizeof(RBTree));

    RBTreeNode* zero = new RBTreeNode(0,0,0);
    writeNode(cmb, 0, zero);
    delete zero;
}

RBTreeNode::RBTreeNode(u_int64_t _node_idx, u_int64_t _color, u_int64_t _parent_idx){
    node_id = _node_idx;
    color = _color;
    parent_idx = _parent_idx;
    left_child_idx = 0;
    right_child_idx = 0;
}

u_int64_t RBTree::insert(CMB* cmb, u_int64_t node_id){
    mylog << "RBTree()::insert() - node_id : " << node_id << endl;
    u_int64_t ret_idx;
    if(root_idx == 0){
        root_idx = get_free_idx(cmb);

        RBTreeNode* new_node = new RBTreeNode(node_id, BLACK, 0);
        writeNode(cmb, root_idx, new_node);

        off_t addr = RED_BLACK_TREE_START_ADDR + ((char*)&root_idx - (char*)this);
        cmb->write(addr, &root_idx, sizeof(u_int64_t));

        delete new_node;
        ret_idx = root_idx;
    }
    else{
        u_int64_t cur_idx = root_idx;
        u_int64_t cur_lower, cur_upper;
        u_int64_t child_ptr, new_idx;
        off_t addr;
        RBTreeNode* rbtree_ref = new RBTreeNode(0,0,0);

        u_int64_t lower = cmb->get_lower(node_id);
        u_int64_t upper = cmb->get_upper(node_id);

        while(cur_idx){

            u_int64_t cur_node_id;
            addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&rbtree_ref->node_id - (char*)rbtree_ref);
            cmb->read(&cur_node_id, addr, sizeof(u_int64_t));

            cur_lower = cmb->get_lower(cur_node_id);
            cur_upper = cmb->get_upper(cur_node_id);

            if(upper < cur_lower){
                addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&rbtree_ref->left_child_idx - (char*)rbtree_ref);
                cmb->read(&child_ptr, addr, sizeof(u_int64_t));
            }                
            else if(lower > cur_upper){
                addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&rbtree_ref->right_child_idx - (char*)rbtree_ref);
                cmb->read(&child_ptr, addr, sizeof(u_int64_t));
            }
            else{
                cout << "Overlapping intervals" << endl;
                mylog << "Overlapping intervals" << endl;
                exit(1);
            }

            if(child_ptr != 0){
                cur_idx = child_ptr;
            }
            else{
                // Write the new node
                new_idx = get_free_idx(cmb);
                RBTreeNode* new_node = new RBTreeNode(node_id, RED, cur_idx);
                writeNode(cmb, new_idx, new_node);      

                // Update the cur->child = new_idx
                if(upper < cur_lower){
                    addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&rbtree_ref->left_child_idx - (char*)rbtree_ref);
                    cmb->write(addr, &new_idx, sizeof(u_int64_t));
                }
                else{
                    addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&rbtree_ref->right_child_idx - (char*)rbtree_ref);
                    cmb->write(addr, &new_idx, sizeof(u_int64_t));
                }

                delete new_node;
                break;
            }
        }
        insertFix(cmb, new_idx);

        delete rbtree_ref;
        ret_idx = new_idx;
    }

    num_of_cache++;
    off_t addr = RED_BLACK_TREE_START_ADDR + ((char*) &num_of_cache - (char*) this);
    cmb->write(addr, &num_of_cache, sizeof(u_int64_t));

    return ret_idx;
}

void RBTree::insertFix(CMB* cmb, u_int64_t idx){
    
    u_int64_t cur_idx = idx;
    while(cur_idx){
        RBTreeNode* cur = new RBTreeNode(0,0,0);
        readNode(cmb, cur_idx, cur);        

        u_int64_t parent_color;
        off_t addr = RED_BLACK_TREE_BASE_ADDR + cur->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
        cmb->read(&parent_color, addr, sizeof(u_int64_t));

        if(parent_color == RED){
            RBTreeNode* parent = new RBTreeNode(0,0,0);
            readNode(cmb, cur->parent_idx, parent);

            u_int64_t grand_left_child;
            addr = RED_BLACK_TREE_BASE_ADDR + parent->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->left_child_idx - (char*)cur);
            cmb->read(&grand_left_child, addr, sizeof(u_int64_t));
            u_int64_t grand_right_child;
            addr = RED_BLACK_TREE_BASE_ADDR + parent->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->right_child_idx - (char*)cur);
            cmb->read(&grand_right_child, addr, sizeof(u_int64_t));

            if(grand_left_child == cur->parent_idx){
                // Parent is on the left
                RBTreeNode* uncle = new RBTreeNode(0,0,0);
                readNode(cmb, grand_right_child, uncle);

                if(uncle->color == RED){
                    // Case 1
                    parent->color = BLACK;
                    addr = RED_BLACK_TREE_BASE_ADDR + cur->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &parent->color, sizeof(u_int64_t));
                    uncle->color = BLACK;
                    addr = RED_BLACK_TREE_BASE_ADDR + grand_right_child * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &uncle->color, sizeof(u_int64_t));
                    // Grandparent->color = RED;
                    if(parent->parent_idx != root_idx){
                        addr = RED_BLACK_TREE_BASE_ADDR + parent->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                        cmb->write(addr, &cur->color, sizeof(u_int64_t));
                    }                   

                    cur_idx = parent->parent_idx;
                }
                else{
                    // Case 2 & 3
                    if(cur_idx == parent->right_child_idx){
                        leafRotation(cmb, cur->parent_idx);
                        cur_idx = cur->parent_idx;
                        readNode(cmb, cur_idx, cur);
                        readNode(cmb, cur->parent_idx, parent);
                    }
                    parent->color = BLACK;
                    addr = RED_BLACK_TREE_BASE_ADDR + cur->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &parent->color, sizeof(u_int64_t));
                    // Grandparent->color = RED;
                    addr = RED_BLACK_TREE_BASE_ADDR + parent->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &cur->color, sizeof(u_int64_t));
                    rightRotation(cmb, parent->parent_idx);

                    cur_idx = 0;
                }
                
                delete uncle;
            }
            else{
                // Parent is on the right
                RBTreeNode* uncle = new RBTreeNode(0,0,0);
                readNode(cmb, grand_left_child, uncle);

                if(uncle->color == RED){
                    // Case 1
                    parent->color = BLACK;
                    addr = RED_BLACK_TREE_BASE_ADDR + cur->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &parent->color, sizeof(u_int64_t));
                    uncle->color = BLACK;
                    addr = RED_BLACK_TREE_BASE_ADDR + grand_left_child * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &uncle->color, sizeof(u_int64_t));
                    // Grandparent->color = RED;
                    if(parent->parent_idx != root_idx){
                        addr = RED_BLACK_TREE_BASE_ADDR + parent->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                        cmb->write(addr, &cur->color, sizeof(u_int64_t));
                    }

                    cur_idx = parent->parent_idx;
                }
                else{
                    // Case 2 & 3
                    if(cur_idx == parent->left_child_idx){
                        rightRotation(cmb, cur->parent_idx);
                        cur_idx = cur->parent_idx;
                        readNode(cmb, cur_idx, cur);
                        readNode(cmb, cur->parent_idx, parent);
                    }
                    parent->color = BLACK;
                    addr = RED_BLACK_TREE_BASE_ADDR + cur->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &parent->color, sizeof(u_int64_t));
                    // Grandparent->color = RED;
                    addr = RED_BLACK_TREE_BASE_ADDR + parent->parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &cur->color, sizeof(u_int64_t));
                    leafRotation(cmb, parent->parent_idx);

                    cur_idx = 0;
                }

                delete uncle;
            }

            delete parent;
        }
        else{
            delete cur;
            return;
        }

        delete cur;
    }
}

void RBTree::remove(CMB* cmb, u_int64_t rbtree_idx){
    mylog << "RBTree::remove() - rbtree_idx = " << rbtree_idx << endl;
    u_int64_t delete_idx = rbtree_idx;
    if(delete_idx == 0){
        cout << "Data not found" << endl;
        return;
    }
    RBTreeNode* delete_node = new RBTreeNode(0,0,0);

    off_t addr;
    u_int64_t target_idx = 0;
    RBTreeNode* target_node = new RBTreeNode(0,0,0);
    u_int64_t replace_idx = 0;

    readNode(cmb, delete_idx, delete_node);

    if(delete_node->left_child_idx == 0 || delete_node->right_child_idx == 0){
        // Has <= 1 child
        target_idx = delete_idx;
        memcpy(target_node, delete_node, sizeof(RBTreeNode));
    }
    else{
        // Has two children
        target_idx = succesor(cmb, delete_node->right_child_idx);
        readNode(cmb, target_idx, target_node);
    }

    // Set the replace_idx as the target's child
    if(target_node->left_child_idx != 0)
        replace_idx = target_node->left_child_idx;
    else
        replace_idx = target_node->right_child_idx;

    if(replace_idx != 0){
        // If the replace_idx is not NIL, set it's parent to target->parent
        addr = RED_BLACK_TREE_BASE_ADDR + replace_idx * sizeof(RBTreeNode) + ((char*)&delete_node->parent_idx - (char*)delete_node);
        cmb->write(addr, &target_node->parent_idx, sizeof(u_int64_t));
    }

    if(target_node->parent_idx == 0){
        // If the target is the root, set the root as the replacement
        addr = RED_BLACK_TREE_START_ADDR + ((char*)&root_idx - (char*)this);
        cmb->write(addr, &replace_idx, sizeof(u_int64_t));
        root_idx = replace_idx;
    }
    else{
        u_int64_t parent_left;
        addr = RED_BLACK_TREE_BASE_ADDR + target_node->parent_idx * sizeof(RBTreeNode) + ((char*)&delete_node->left_child_idx - (char*)delete_node);
        cmb->read(&parent_left, addr, sizeof(u_int64_t));

        if(parent_left == target_idx){
            // If target is the left child, set the replace to the left child of target->parent
            cmb->write(addr, &replace_idx, sizeof(u_int64_t));
        }
        else{
            addr = RED_BLACK_TREE_BASE_ADDR + target_node->parent_idx * sizeof(RBTreeNode) + ((char*)&delete_node->right_child_idx - (char*)delete_node);
            cmb->write(addr, &replace_idx, sizeof(u_int64_t));
        }
    }

    if(target_idx != delete_idx){
        // Replace the succesor node id to the internal node
        addr = RED_BLACK_TREE_BASE_ADDR + delete_idx * sizeof(RBTreeNode) + ((char*)&delete_node->node_id - (char*)delete_node);
        cmb->write(addr, &target_node->node_id, sizeof(u_int64_t));
        // Update the LRU -> RBTree relationship
        BKMap* BKMap_ref = new BKMap();
        LRU_QUEUE_ENTRY* LRU_ref = new LRU_QUEUE_ENTRY();        
        u_int64_t LRU_idx;

        addr = BLOCK_MAPPING_START_ADDR + target_node->node_id * sizeof(BKMap) + ((char*)&BKMap_ref->lru_id - (char*)BKMap_ref);
        cmb->read(&LRU_idx, addr, sizeof(u_int64_t));
        addr = LRU_QUEUE_BASE_ADDR + LRU_idx * sizeof(LRU_QUEUE_ENTRY) + ((char*)&LRU_ref->RBTREE_idx - (char*)LRU_ref);
        cmb->write(addr, &delete_idx, sizeof(u_int64_t));

        delete BKMap_ref;   
        delete LRU_ref;      
    }

    if(target_node->color == BLACK){
        removeFix(cmb, replace_idx, target_node->parent_idx);
    }

    free_push(cmb, target_idx);

    num_of_cache--;
    addr = RED_BLACK_TREE_START_ADDR + ((char*) &num_of_cache - (char*) this);
    cmb->write(addr, &num_of_cache, sizeof(u_int64_t));
    
    delete delete_node;
    delete target_node;
}

void RBTree::removeFix(CMB* cmb, u_int64_t idx, u_int64_t cur_parent_idx){
    u_int64_t cur_idx = idx;
    u_int64_t parent_idx = cur_parent_idx;
    u_int64_t red = RED;
    u_int64_t black = BLACK;
    while(true){
        off_t addr; 
        RBTreeNode* cur = new RBTreeNode(0,0,0);
        readNode(cmb, cur_idx, cur);

        if(cur_idx != root_idx && cur->color == BLACK){
            RBTreeNode* parent = new RBTreeNode(0,0,0);
            readNode(cmb, parent_idx, parent);

            if(cur_idx == parent->left_child_idx){
                // Current is the left child
                RBTreeNode* sibling = new RBTreeNode(0,0,0);
                u_int64_t sibling_idx = parent->right_child_idx;
                readNode(cmb, sibling_idx, sibling);

                if(sibling->color == RED){
                    //Case 1
                    sibling->color = BLACK;
                    addr = RED_BLACK_TREE_BASE_ADDR + sibling_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &black, sizeof(u_int64_t));
                    parent->color = RED;
                    addr = RED_BLACK_TREE_BASE_ADDR + parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &red, sizeof(u_int64_t));
                    leafRotation(cmb, parent_idx);
                    readNode(cmb, parent_idx, parent);
                    sibling_idx = parent->right_child_idx;
                    readNode(cmb, sibling_idx, sibling);
                }

                RBTreeNode* sibling_left_child = new RBTreeNode(0,0,0);
                readNode(cmb, sibling->left_child_idx, sibling_left_child);
                RBTreeNode* sibling_right_child = new RBTreeNode(0,0,0);
                readNode(cmb, sibling->right_child_idx, sibling_right_child);

                if(sibling_left_child->color == BLACK && sibling_right_child->color == BLACK){
                    // Case 2
                    sibling->color = RED;
                    addr = RED_BLACK_TREE_BASE_ADDR + sibling_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &red, sizeof(u_int64_t));
                    cur_idx = parent_idx;
                    addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&cur->parent_idx - (char*)cur);
                    cmb->read(&parent_idx, addr, sizeof(u_int64_t));
                }
                else{
                    if(sibling_right_child->color == BLACK){
                        // Case 3
                        sibling_left_child->color = BLACK;
                        addr = RED_BLACK_TREE_BASE_ADDR + sibling->left_child_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                        cmb->write(addr, &black, sizeof(u_int64_t));
                        sibling->color = RED;
                        addr = RED_BLACK_TREE_BASE_ADDR + sibling_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                        cmb->write(addr, &red, sizeof(u_int64_t));
                        rightRotation(cmb, sibling_idx);
                        readNode(cmb, parent_idx, parent);
                        sibling_idx = parent->right_child_idx;
                        readNode(cmb, sibling_idx, sibling);
                    }

                    // Case 4
                    sibling->color = parent->color;
                    addr = RED_BLACK_TREE_BASE_ADDR + sibling_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &parent->color, sizeof(u_int64_t));
                    parent->color = BLACK;
                    addr = RED_BLACK_TREE_BASE_ADDR + parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &black, sizeof(u_int64_t));
                    // sibling->right->color = BLACK
                    addr = RED_BLACK_TREE_BASE_ADDR + sibling->right_child_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &black, sizeof(u_int64_t));
                    leafRotation(cmb, parent_idx);
                    cur_idx = root_idx;
                }    

                delete sibling;    
                delete sibling_left_child;
                delete sibling_right_child;        
            }
            else{
                // Current is the right child
                RBTreeNode* sibling = new RBTreeNode(0,0,0);
                u_int64_t sibling_idx = parent->left_child_idx;
                readNode(cmb, sibling_idx, sibling);

                if(sibling->color == RED){
                    //Case 1
                    sibling->color = BLACK;
                    addr = RED_BLACK_TREE_BASE_ADDR + sibling_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &black, sizeof(u_int64_t));
                    parent->color = RED;
                    addr = RED_BLACK_TREE_BASE_ADDR + parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &red, sizeof(u_int64_t));
                    rightRotation(cmb, parent_idx);
                    readNode(cmb, parent_idx, parent);
                    sibling_idx = parent->left_child_idx;
                    readNode(cmb, sibling_idx, sibling);
                }

                RBTreeNode* sibling_left_child = new RBTreeNode(0,0,0);
                readNode(cmb, sibling->left_child_idx, sibling_left_child);
                RBTreeNode* sibling_right_child = new RBTreeNode(0,0,0);
                readNode(cmb, sibling->right_child_idx, sibling_right_child);

                if(sibling_left_child->color == BLACK && sibling_right_child->color == BLACK){
                    // Case 2
                    sibling->color = RED;
                    addr = RED_BLACK_TREE_BASE_ADDR + sibling_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &red, sizeof(u_int64_t));
                    cur_idx = parent_idx;
                    addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&cur->parent_idx - (char*)cur);
                    cmb->read(&parent_idx, addr, sizeof(u_int64_t));
                }
                else{
                    if(sibling_left_child->color == BLACK){
                        // Case 3
                        sibling_right_child->color = BLACK;
                        addr = RED_BLACK_TREE_BASE_ADDR + sibling->right_child_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                        cmb->write(addr, &black, sizeof(u_int64_t));
                        sibling->color = RED;
                        addr = RED_BLACK_TREE_BASE_ADDR + sibling_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                        cmb->write(addr, &red, sizeof(u_int64_t));
                        leafRotation(cmb, sibling_idx);
                        readNode(cmb, parent_idx, parent);
                        sibling_idx = parent->left_child_idx;
                        readNode(cmb, sibling_idx, sibling);
                    }

                    // Case 4
                    sibling->color = parent->color;
                    addr = RED_BLACK_TREE_BASE_ADDR + sibling_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &parent->color, sizeof(u_int64_t));
                    parent->color = BLACK;
                    addr = RED_BLACK_TREE_BASE_ADDR + parent_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &black, sizeof(u_int64_t));
                    // sibling->left->color = BLACK
                    addr = RED_BLACK_TREE_BASE_ADDR + sibling->left_child_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
                    cmb->write(addr, &black, sizeof(u_int64_t));
                    rightRotation(cmb, parent_idx);
                    cur_idx = root_idx;
                }   

                delete sibling;    
                delete sibling_left_child;
                delete sibling_right_child;   
            }
            delete parent;
        }
        else{
            addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&cur->color - (char*)cur);
            cmb->write(addr, &black, sizeof(u_int64_t));
            delete cur;
            break;
        }  

        delete cur;   
    }
}

u_int64_t RBTree::search(CMB* cmb, u_int64_t key){
    mylog << "RBTree::search() - key = " << key << endl;
    u_int64_t cur_idx = root_idx;
    RBTreeNode* rbtree_ref = new RBTreeNode(0,0,0);

    off_t addr;
    u_int64_t cur_lower, cur_upper;

    while(cur_idx){
        u_int64_t node_id;
        addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&rbtree_ref->node_id - (char*)rbtree_ref);
        cmb->read(&node_id, addr, sizeof(u_int64_t));

        cur_lower = cmb->get_lower(node_id);
        cur_upper = cmb->get_upper(node_id);

        if(key >= cur_lower && key <= cur_upper){
            delete rbtree_ref;
            return node_id;
        }
        else if(key < cur_lower){
            addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode)  + ((char*)&rbtree_ref->left_child_idx - (char*)rbtree_ref);
            cmb->read(&cur_idx, addr, sizeof(u_int64_t));
        }
        else{
            addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&rbtree_ref->right_child_idx - (char*)rbtree_ref);
            cmb->read(&cur_idx, addr, sizeof(u_int64_t));
        }
    }

    delete rbtree_ref;
    return 0;
}

void RBTree::stat(CMB* cmb){
    RBTree* meta = (RBTree*) malloc(sizeof(RBTree));

    off_t addr = RED_BLACK_TREE_START_ADDR;
    cmb->read(meta, addr, sizeof(RBTree));

    cout << "RBTree: stat()" << endl;
    cout << "root_idx = " << meta->root_idx << endl;
    cout << "free_Stack_idx = " << meta->free_Stack_idx << endl;
    cout << "free_idx = " << meta->free_idx << endl;
    cout << "capacity = " << meta->capacity << endl;
    cout << "num_of_cache = " << meta->num_of_cache << endl;
    
    display(cmb);

    free(meta);
}

void RBTree::display(CMB* cmb){
    if(root_idx != 0){
        RBTreeNode* node = new RBTreeNode(0,0,0);
        off_t addr = RED_BLACK_TREE_BASE_ADDR + root_idx * sizeof(RBTreeNode);
        cmb->read(node, addr, sizeof(RBTreeNode));

        node->display(cmb, root_idx, 0);

        delete node;
    }
}

void RBTreeNode::display(CMB* cmb, u_int64_t idx, int level){
    if(right_child_idx != 0){
        RBTreeNode* node = new RBTreeNode(0,0,0);
        off_t addr = RED_BLACK_TREE_BASE_ADDR + right_child_idx * sizeof(RBTreeNode);
        cmb->read(node, addr, sizeof(RBTreeNode));
        
        node->display(cmb, right_child_idx, level + 1);
        delete node;
    }

    for(int i = 0; i < level; i++)
        cout << "\t";
    string color_code = (color == RED) ? "Red" : "Black";
    cout << "(" << idx << "->" << node_id << ": " << cmb->get_lower(node_id) << "-" << cmb->get_upper(node_id) << ":" << color_code << ")" << endl;
    

    if(left_child_idx != 0){
        RBTreeNode* node = new RBTreeNode(0,0,0);
        off_t addr = RED_BLACK_TREE_BASE_ADDR + left_child_idx * sizeof(RBTreeNode);
        cmb->read(node, addr, sizeof(RBTreeNode));

        node->display(cmb, left_child_idx, level + 1);
        delete node;
    }
}

u_int64_t RBTree::get_free_idx(CMB* cmb){
    // If the stack is empty
    if(free_Stack_idx == 0){
        off_t ret_idx = free_idx;
        free_idx++;

        // Write the new free_idx back
        off_t addr = RED_BLACK_TREE_START_ADDR + ((char*) &free_idx - (char*) this);
        cmb->write(addr, &free_idx, sizeof(u_int64_t));

        return ret_idx;
    }
    else
        return free_pop(cmb);
}

void RBTree::free_push(CMB* cmb, u_int64_t idx){
    // Set the new head->next as the current head idx
    off_t addr = RED_BLACK_TREE_BASE_ADDR + idx * sizeof(RBTreeNode);
    if(addr >= RED_BLACK_TREE_END_ADDR){
        cout << "Red Black Tree Node Limit Exceed" << endl;
        mylog << "Red Black Tree Node Limit Exceed" << endl;
        exit(1);
    }
    cmb->write(addr, &free_Stack_idx, sizeof(u_int64_t));

    // Set the stack head to the new head
    addr = RED_BLACK_TREE_START_ADDR + ((char*)&free_Stack_idx - (char*)this);
    cmb->write(addr, &idx, sizeof(u_int64_t)); 

    free_Stack_idx = idx;
}

u_int64_t RBTree::free_pop(CMB* cmb){
    u_int64_t ret, next;

    if(free_Stack_idx == 0)
        return 0;

    // Return value would be the stack head
    ret = free_Stack_idx;
    
    // Read the new stack head from the head idx
    off_t addr = RED_BLACK_TREE_BASE_ADDR + free_Stack_idx * sizeof(RBTreeNode);
    if(addr >= RED_BLACK_TREE_END_ADDR){
        cout << "Red Black Tree Node Limit Excced" << endl;
        mylog << "Red Black Tree Node Limit Excced" << endl;
        exit(1);
    }
    cmb->read(&next, addr, sizeof(u_int64_t));

    // Write the new stack head back
    free_Stack_idx = next;

    addr = RED_BLACK_TREE_START_ADDR + ((char*)&free_Stack_idx - (char*)this);
    cmb->write(addr, &free_Stack_idx, sizeof(u_int64_t));

    return ret;
}

void RBTree::readNode(CMB* cmb, u_int64_t idx, RBTreeNode* buf){
    off_t addr = RED_BLACK_TREE_BASE_ADDR + idx * sizeof(RBTreeNode);
    cmb->read(buf, addr, sizeof(RBTreeNode));
}

void RBTree::writeNode(CMB* cmb, u_int64_t idx, RBTreeNode* buf){
    off_t addr = RED_BLACK_TREE_BASE_ADDR + idx * sizeof(RBTreeNode);
    cmb->write(addr, buf, sizeof(RBTreeNode));
}

void RBTree::leafRotation(CMB* cmb, u_int64_t x_idx){
    off_t addr;
    bool root = false;
    RBTreeNode* x = new RBTreeNode(0,0,0);
    readNode(cmb, x_idx, x);
    u_int64_t y_idx = x->right_child_idx;
    RBTreeNode* y = new RBTreeNode(0,0,0);
    readNode(cmb, x->right_child_idx, y);
    RBTreeNode* parent = new RBTreeNode(0,0,0);
    if(x_idx == root_idx)
        root = true;
    else
        readNode(cmb, x->parent_idx, parent);

    x->right_child_idx = y->left_child_idx;
    addr = RED_BLACK_TREE_BASE_ADDR + x_idx * sizeof(RBTreeNode) + ((char*)&x->right_child_idx - (char*)x);
    cmb->write(addr, &y->left_child_idx, sizeof(u_int64_t));
    // y->leaf_child->parent = x
    addr = RED_BLACK_TREE_BASE_ADDR + y->left_child_idx * sizeof(RBTreeNode) + ((char*)&x->parent_idx - (char*)x);
    cmb->write(addr, &x_idx, sizeof(u_int64_t));

    // x->parent->child = y
    if(root){
        addr = RED_BLACK_TREE_START_ADDR + ((char*)&root_idx - (char*)this);
        cmb->write(addr, &y_idx, sizeof(u_int64_t));
        root_idx = y_idx;
    }
    else{
        if(parent->left_child_idx == x_idx)
            addr = RED_BLACK_TREE_BASE_ADDR + x->parent_idx * sizeof(RBTreeNode) + ((char*)&x->left_child_idx - (char*)x);
        else
            addr = RED_BLACK_TREE_BASE_ADDR + x->parent_idx * sizeof(RBTreeNode) + ((char*)&x->right_child_idx - (char*)x);
        cmb->write(addr, &y_idx, sizeof(u_int64_t));  
    }  
    
    y->parent_idx = x->parent_idx;
    addr = RED_BLACK_TREE_BASE_ADDR + y_idx * sizeof(RBTreeNode) + ((char*)&x->parent_idx - (char*)x);
    cmb->write(addr, &x->parent_idx, sizeof(u_int64_t));

    y->left_child_idx = x_idx;
    addr = RED_BLACK_TREE_BASE_ADDR + y_idx * sizeof(RBTreeNode) + ((char*)&x->left_child_idx - (char*)x);
    cmb->write(addr, &x_idx, sizeof(u_int64_t));
    x->parent_idx = y_idx;
    addr = RED_BLACK_TREE_BASE_ADDR + x_idx * sizeof(RBTreeNode) + ((char*)&x->parent_idx - (char*)x);
    cmb->write(addr, &y_idx, sizeof(u_int64_t));

    delete x;
    delete y;
    delete parent;
}

void RBTree::rightRotation(CMB* cmb, u_int64_t x_idx){
    off_t addr;
    bool root = false;
    RBTreeNode* x = new RBTreeNode(0,0,0);
    readNode(cmb, x_idx, x);
    u_int64_t y_idx = x->left_child_idx;
    RBTreeNode* y = new RBTreeNode(0,0,0);
    readNode(cmb, x->left_child_idx, y);
    RBTreeNode* parent = new RBTreeNode(0,0,0);
    if(x_idx == root_idx)
        root = true;
    else
        readNode(cmb, x->parent_idx, parent);

    x->left_child_idx = y->right_child_idx;
    addr = RED_BLACK_TREE_BASE_ADDR + x_idx * sizeof(RBTreeNode) + ((char*)&x->left_child_idx - (char*)x);
    cmb->write(addr, &y->right_child_idx, sizeof(u_int64_t));
    // y->leaf_child->parent = x
    addr = RED_BLACK_TREE_BASE_ADDR + y->right_child_idx * sizeof(RBTreeNode) + ((char*)&x->parent_idx - (char*)x);
    cmb->write(addr, &x_idx, sizeof(u_int64_t));

    // x->parent->child = y
    if(root){
        addr = RED_BLACK_TREE_START_ADDR + ((char*)&root_idx - (char*)this);
        cmb->write(addr, &y_idx, sizeof(u_int64_t));
        root_idx = y_idx;
    }
    else{
        if(parent->left_child_idx == x_idx)
            addr = RED_BLACK_TREE_BASE_ADDR + x->parent_idx * sizeof(RBTreeNode) + ((char*)&x->left_child_idx - (char*)x);
        else
            addr = RED_BLACK_TREE_BASE_ADDR + x->parent_idx * sizeof(RBTreeNode) + ((char*)&x->right_child_idx - (char*)x);
        cmb->write(addr, &y_idx, sizeof(u_int64_t));
    }
    
    y->parent_idx = x->parent_idx;
    addr = RED_BLACK_TREE_BASE_ADDR + y_idx * sizeof(RBTreeNode) + ((char*)&x->parent_idx - (char*)x);
    cmb->write(addr, &x->parent_idx, sizeof(u_int64_t));

    y->right_child_idx = x_idx;
    addr = RED_BLACK_TREE_BASE_ADDR + y_idx * sizeof(RBTreeNode) + ((char*)&x->right_child_idx - (char*)x);
    cmb->write(addr, &x_idx, sizeof(u_int64_t));
    x->parent_idx = y_idx;
    addr = RED_BLACK_TREE_BASE_ADDR + x_idx * sizeof(RBTreeNode) + ((char*)&x->parent_idx - (char*)x);
    cmb->write(addr, &y_idx, sizeof(u_int64_t));

    delete x;
    delete y;
    delete parent;
}

u_int64_t RBTree::succesor(CMB* cmb, u_int64_t idx){
    u_int64_t cur_idx = idx;
    
    while(cur_idx){
        RBTreeNode* ref = new RBTreeNode(0,0,0);
        u_int64_t left_child;
        off_t addr = RED_BLACK_TREE_BASE_ADDR + cur_idx * sizeof(RBTreeNode) + ((char*)&ref->left_child_idx - (char*)ref);
        cmb->read(&left_child, addr, sizeof(u_int64_t));

        if(left_child){
            cur_idx = left_child;
            delete ref;
        }
        else{
            delete ref;
            return cur_idx;
        }
    }

    return 0;
}

template <typename T>
void APPEND<T>::append_entry(BTree<T>* t, u_int64_t node_id, OPR_CODE OPR, u_int64_t _k, T _v){
    mylog << "APPEND::append_entry()" << endl;

    if(full(t->cmb, node_id)){
        // Reduction, write to the node
        BTreeNode<T>* node = new BTreeNode<T>(0,0,0);
        t->node_read(node_id, node);
        reduction(t, node_id, node);
        t->node_write(node_id, node);
        t->append_map->delete_entries(t->cmb, node_id);
        delete node;
    }

    APPEND_ENTRY* new_entry = new APPEND_ENTRY();
    new_entry->opr = (u_int64_t) OPR;
    new_entry->key = _k;
    if(OPR == I || OPR == U){
        new_entry->value_idx = value_pool->get_free_idx(t->cmb);
        value_pool->write_value(t->cmb, new_entry->value_idx, &_v);
    }

    u_int64_t num = get_num(t->cmb, node_id);
    off_t addr = APPEND_OPR_START_ADDR + node_id * (sizeof(u_int64_t) + NUM_OF_APPEND * sizeof(APPEND_ENTRY));
    addr += sizeof(u_int64_t) + num * sizeof(APPEND_ENTRY);
    t->cmb->write(addr, new_entry, sizeof(APPEND_ENTRY));

    write_num(t->cmb, node_id, num+1);

    delete new_entry;
}

template <typename T>
bool APPEND<T>::full(CMB* cmb, u_int64_t node_id){
    mylog << "APPEND::full() - node_id = " << node_id << endl;

    u_int64_t num = get_num(cmb, node_id);
    if(num >= NUM_OF_APPEND)
        return true;
    else
        return false;
}

template <typename T>
void APPEND<T>::reduction(BTree<T>* t, u_int64_t node_id, BTreeNode<T>* node){
    mylog << "APPEND::reduction() - node_id = " << node_id << endl;

    u_int64_t entry_num = get_num(t->cmb, node_id);
    for(u_int64_t idx = 1; idx <= entry_num; idx++){
        OPR_CODE opr = get_opr(t->cmb, node_id, idx);

        if(opr == I){
            u_int64_t _k = get_key(t->cmb, node_id, idx);
            T _v = get_value(t->cmb, node_id, idx);

            int i;
            bool ins = true ;
            for(i = 0; i < node->num_key; i++){
                if(_k == node->key[i]){
                    ins = false;
                    break;
                }
                else{
                    if(_k < node->key[i])
                        break;
                }
            }
            if(ins){
                for(int j = node->num_key; j > i; j--){
                    node->key[j] = node->key[j-1];
                    node->value[j] = node->value[j-1];
                }
                node->key[i] = _k;
                node->value[i] = _v;
                node->num_key++;
            }
        }
        else if(opr == U){
            u_int64_t _k = get_key(t->cmb, node_id, idx);
            T _v = get_value(t->cmb, node_id, idx);

            for(int i = 0; i < node->num_key; i++){
                if(_k == node->key[i])
                    node->value[i] = _v;
                if(_k < node->key[i])
                    break;
            }
        }
        else{
            cout << "Incorrect OPR" << endl;
            mylog << "Incorrect OPR" << endl;
            exit(1);
        }
    }
}

template <typename T>
u_int64_t APPEND<T>::search_entry(CMB* cmb, u_int64_t node_id, u_int64_t key){
    mylog << "APPEND::search_entry() - node_id = " << node_id << endl;

    u_int64_t num = get_num(cmb, node_id);
    while(num){
        u_int64_t ret = get_key(cmb, node_id, num);
        if(key == ret)
            return num;
        num--;
    }
    return 0;
}

template <typename T>
u_int64_t APPEND<T>::get_num(CMB* cmb, u_int64_t node_id){
    mylog << "APPEND::get_num() - node_id = " << node_id << endl;
    
    off_t addr = APPEND_OPR_START_ADDR + node_id * (sizeof(u_int64_t) + NUM_OF_APPEND * sizeof(APPEND_ENTRY));
    u_int64_t ret;
    cmb->read(&ret, addr, sizeof(u_int64_t));
    return ret;
}

template <typename T>
void APPEND<T>::write_num(CMB* cmb, u_int64_t node_id, u_int64_t num){
    mylog << "APPEND::write_num() - node_id = " << node_id << ", num = " << num << endl;

    off_t addr = APPEND_OPR_START_ADDR + node_id * (sizeof(u_int64_t) + NUM_OF_APPEND * sizeof(APPEND_ENTRY));
    cmb->write(addr, &num, sizeof(u_int64_t));
}

template <typename T>
OPR_CODE APPEND<T>::get_opr(CMB* cmb, u_int64_t node_id, u_int64_t idx){
    mylog << "APPEND::get_opr() - node_id = " << node_id << ", idx = " << idx << endl;

    APPEND_ENTRY ref;
    u_int64_t ret;
    off_t addr = APPEND_OPR_START_ADDR + node_id * (sizeof(u_int64_t) + NUM_OF_APPEND * sizeof(APPEND_ENTRY));
    addr += sizeof(u_int64_t) + (idx-1) * sizeof(APPEND_ENTRY) + ((char*)&ref.opr - (char*)&ref);
    cmb->read(&ret, addr, sizeof(u_int64_t));

    return (OPR_CODE) ret;
}

template <typename T>
u_int64_t APPEND<T>::get_key(CMB* cmb, u_int64_t node_id, u_int64_t idx){
    mylog << "APPEND::get_key() - node_id = " << node_id << ", idx = " << idx << endl;

    APPEND_ENTRY ref;
    u_int64_t ret;
    off_t addr = APPEND_OPR_START_ADDR + node_id * (sizeof(u_int64_t) + NUM_OF_APPEND * sizeof(APPEND_ENTRY));
    addr += sizeof(u_int64_t) + (idx-1) * sizeof(APPEND_ENTRY) + ((char*)&ref.key - (char*)&ref);
    cmb->read(&ret, addr, sizeof(u_int64_t));

    return ret;
}

template <typename T>
T APPEND<T>::get_value(CMB* cmb, u_int64_t node_id, u_int64_t idx){
    mylog << "APPEND::get_value() - node_id = " << node_id << ", idx = " << idx << endl;

    APPEND_ENTRY ref;
    u_int64_t value_idx;
    T ret;
    off_t addr = APPEND_OPR_START_ADDR + node_id * (sizeof(u_int64_t) + NUM_OF_APPEND * sizeof(APPEND_ENTRY));
    addr += sizeof(u_int64_t) + (idx-1) * sizeof(APPEND_ENTRY) + ((char*)&ref.value_idx - (char*)&ref);
    cmb->read(&value_idx, addr, sizeof(u_int64_t));

    value_pool->get_value(cmb, value_idx, &ret);

    return ret;
}

template <typename T>
void APPEND<T>::delete_entries(CMB* cmb, u_int64_t node_id){
    mylog << "APPEND::delete_entries() - node_id = " << node_id << endl;

    APPEND_ENTRY ref;
    u_int64_t num = get_num(cmb, node_id);
    while(num){
        OPR_CODE opr = get_opr(cmb, node_id, num);
        if(opr == I or opr == U){
            u_int64_t value_idx;
            off_t addr = APPEND_OPR_START_ADDR + node_id * (sizeof(u_int64_t) + NUM_OF_APPEND * sizeof(APPEND_ENTRY));
            addr += sizeof(u_int64_t) + (num-1) * sizeof(APPEND_ENTRY) + ((char*)&ref.value_idx - (char*)&ref);
            cmb->read(&value_idx, addr, sizeof(u_int64_t));

            value_pool->free_push(cmb, value_idx);
        }
        num--;
    }
    write_num(cmb, node_id, 0);
}

template <typename T>
VALUE_POOL<T>::VALUE_POOL(CMB* cmb){
    free_Stack_idx = 0;
    free_idx = 0;
    num = 0;
    capacity = (VALUE_POOL_END_ADDR - VALUE_POOL_BASE_ADDR) / sizeof(T) - 1;

    off_t addr = VALUE_POOL_START_ADDR;
    cmb->write(addr, this, sizeof(VALUE_POOL<T>));
}

template <typename T>
void VALUE_POOL<T>::stat(CMB* cmb){
    VALUE_POOL<T>* meta = (VALUE_POOL<T>*) malloc(sizeof(VALUE_POOL<T>));

    off_t addr = VALUE_POOL_START_ADDR;
    cmb->read(meta, addr, sizeof(VALUE_POOL<T>));

    cout << "Free_Stack_idx = " << free_Stack_idx << endl;
    cout << "free_idx = " << free_idx << endl;
    cout << "capacity = " << capacity << endl;
    cout << "num = " << num << endl;

    free(meta);
}

template <typename T>
u_int64_t VALUE_POOL<T>::get_free_idx(CMB* cmb){
    mylog << "VALUE_POOL::get_free_idx()" << endl;
    // If the stack is empty
    if(free_Stack_idx == 0){
        off_t ret_idx = free_idx;
        free_idx++;

        // Write the new free_idx back
        off_t addr = VALUE_POOL_START_ADDR + ((char*) &free_idx - (char*) this);
        cmb->write(addr, &free_idx, sizeof(u_int64_t));

        return ret_idx;
    }
    else
        return free_pop(cmb);
}

template <typename T>
void VALUE_POOL<T>::free_push(CMB* cmb, u_int64_t idx){
    mylog << "VALUE_POOL::free_push(): idx = " << idx << endl;
    // Set the new head->next as the current head idx
    off_t addr = VALUE_POOL_BASE_ADDR + idx * sizeof(T);
    if(addr >= VALUE_POOL_END_ADDR){
        cout << "Value Pool Limit Exceed" << endl;
        mylog << "Value Pool Limit Exceed" << endl;
        exit(1);
    }
    cmb->write(addr, &free_Stack_idx, sizeof(u_int64_t));

    // Set the stack head to the new head
    addr = VALUE_POOL_START_ADDR + ((char*)&free_Stack_idx - (char*)this);
    cmb->write(addr, &idx, sizeof(u_int64_t)); 

    free_Stack_idx = idx;
}

template <typename T>
u_int64_t VALUE_POOL<T>::free_pop(CMB* cmb){
    mylog << "VALUE_POOL::free_pop()" << endl;
    u_int64_t ret, next;

    if(free_Stack_idx == 0)
        return 0;

    // Return value would be the stack head
    ret = free_Stack_idx;
    
    // Read the new stack head from the head idx
    off_t addr = VALUE_POOL_BASE_ADDR + free_Stack_idx * sizeof(T);
    if(addr >= VALUE_POOL_END_ADDR){
        cout << "Value Pool Limit Excced" << endl;
        mylog << "Value Pool Limit Excced" << endl;
        exit(1);
    }
    cmb->read(&next, addr, sizeof(u_int64_t));

    // Write the new stack head back
    free_Stack_idx = next;

    addr = VALUE_POOL_START_ADDR + ((char*)&free_Stack_idx - (char*)this);
    cmb->write(addr, &free_Stack_idx, sizeof(u_int64_t));

    return ret;
}

template <typename T>
void VALUE_POOL<T>::get_value(CMB* cmb, u_int64_t idx, T* buf){
    off_t addr = VALUE_POOL_BASE_ADDR + idx * sizeof(T);
    if(addr >= VALUE_POOL_END_ADDR){
        cout << "Value Pool Limit Excced" << endl;
        mylog << "Value Pool Limit Excced" << endl;
        exit(1);
    }
    cmb->read(buf, addr, sizeof(T));
}

template <typename T>
void VALUE_POOL<T>::write_value(CMB* cmb, u_int64_t idx, T* buf){
    off_t addr = VALUE_POOL_BASE_ADDR + idx * sizeof(T);
    if(addr >= VALUE_POOL_END_ADDR){
        cout << "Value Pool Limit Excced" << endl;
        mylog << "Value Pool Limit Excced" << endl;
        exit(1);
    }
    cmb->write(addr, buf, sizeof(T));
}


#endif /* B_TREE_H */
