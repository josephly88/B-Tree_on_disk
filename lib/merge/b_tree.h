#ifndef B_TREE_H //include guard
#define B_TREE_H

#include <iostream>
#include <fstream>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <chrono>
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

bool needsplit;
bool hb_block;

// Evaluation Break Down
chrono::duration<double, micro> flash_diff;
chrono::duration<double, micro> cmb_diff;

size_t kv_size;
size_t flash_read_size;
size_t flash_write_size;
size_t cmb_read_size;
size_t cmb_write_size;
char structural_change;

// B-Tree
template <typename T> class BTree;
template <typename T> class BTreeNode;
class removeList;
// CMB
template <typename T> class CMB;
class meta_BKMap;
class BKMap;
// Append operation
class LRU_list_entry;
class node_LRU;
class APPEND_ENTRY;
class IU_LIST;
// HBTree
class HB_LRU_list_entry;
class HB_LRU;


template <typename T>
class BTree{
	int m;				// degree
    int min_num; 		// minimun number of node
	u_int64_t root_id;		// Pointer to root node
	int fd;
	int block_size;
	int block_cap;
    int height;
    MODE mode;
		
	public:
	    CMB<T>* cmb;

		BTree(char* filename, int degree, MODE _mode, int append, int read_threshold, bool LRU, int HB);
		~BTree();

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

        // HBTree
        void hb_force_merge(u_int64_t _k, u_int64_t cmb_id);
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

		void search(BTree<T>* t, u_int64_t _k, T* buf, removeList** list);
        u_int64_t update(BTree<T>* t, u_int64_t _k, T _v, removeList** list);

		u_int64_t traverse_insert(BTree<T>* t, u_int64_t _k, T _v, removeList** list);
		u_int64_t direct_insert(BTree<T>* t, u_int64_t _k, T _v, removeList** list, u_int64_t node_id1 = 0, u_int64_t node_id2 = 0);
		u_int64_t split(BTree<T>* t, u_int64_t node_id, u_int64_t parent_id, removeList** list);

		u_int64_t traverse_delete(BTree<T>* t, u_int64_t _k, removeList** list);
		u_int64_t direct_delete(BTree<T>* t, u_int64_t _k, removeList** list);
		u_int64_t rebalance(BTree<T>* t, int idx, removeList** list);
		u_int64_t get_pred(BTree<T>* t);
		u_int64_t get_succ(BTree<T>* t);	

        // HBTree
        u_int64_t hb_force_merge(BTree<T>* t, u_int64_t _k, u_int64_t cmb_id, removeList** list);
};

class removeList{
	public:
		int id;
		removeList* next;
		removeList(int _id, removeList* _next);
};

template <typename T>
class CMB{
	int fd;
    off_t bar_addr;
	void* map_base;
    u_int64_t map_idx;
    void* cache_base;
    MODE mode;

	public:
        int MAX_NUM_IU;
        int read_threshold;
        node_LRU* nodeLRU;
        HB_LRU* hbLRU; 
        int opt;

		CMB(MODE _mode);
		~CMB();

        void* get_map_base();

        void cmb_memcpy(void* dest, void* src, size_t len);
        void remap(off_t offset);

		void read(void* buf, off_t offset, int size);
		void write(off_t offset, void* buf, int size);

        // BKMap
        void addr_bkmap_check(off_t addr);
        void addr_bkmap_meta_check(off_t addr);
		u_int64_t get_new_node_id();
        void new_node_id_init();
		u_int64_t get_block_id(u_int64_t node_id);
		void update_node_id(u_int64_t node_id, u_int64_t block_id);
        // Meta data
        u_int64_t get_num_kv(u_int64_t node_id);
        void update_num_kv(u_int64_t node_id, u_int64_t value);
        u_int64_t get_is_leaf(u_int64_t node_id);
        void update_is_leaf(u_int64_t node_id, u_int64_t value);

        u_int64_t get_iu_ptr(u_int64_t node_id);
        void update_iu_ptr(u_int64_t node_id, u_int64_t value);

        // Append Meta
        u_int64_t get_num_iu();
        void update_num_iu(u_int64_t num);
        u_int64_t get_clear_ptr();
        void set_clear_ptr(u_int64_t node_id);

        u_int64_t get_free_iu_stack_id();
        void update_free_iu_stack_id(u_int64_t value);
        u_int64_t get_free_iu_id();
        void update_free_iu_id(u_int64_t value);

        u_int64_t get_free_val_stack_id();
        void update_free_val_stack_id(u_int64_t value);
        u_int64_t get_free_val_id();
        void update_free_val_id(u_int64_t value);

        u_int64_t pop_iu_id(); 
        void push_iu_id(u_int64_t iu_id); 
        u_int64_t pop_val_id(); 
        void push_val_id(u_int64_t val_id); 

        // Public Function
        void append(BTree<T>* t, u_int64_t node_id, OPR_CODE OPR, u_int64_t _k, T _v, removeList** list);
        void reduction(u_int64_t node_id, BTreeNode<T>* node);
        void clear_iu(u_int64_t node_id);

        void append_entry(u_int64_t node_id, OPR_CODE OPR, u_int64_t _k, T _v);
        bool full(); 
        void reduction_create_iu_list(u_int64_t node_id, IU_LIST** iu_stack);
        void reduction_insert(BTreeNode<T>* node, IU_LIST* iu_stack);
        void reduction_update(BTreeNode<T>* node, IU_LIST* iu_stack);
        void reduction_delete(BTreeNode<T>* node, IU_LIST* iu_stack);

        u_int64_t search_entry(u_int64_t node_id, u_int64_t _k, BTree<T>* t, removeList** list);

        void addr_IU_check(off_t addr);
        void addr_value_check(off_t addr);
        void iu_write_iu(u_int64_t iu_id, APPEND_ENTRY iu);
        OPR_CODE iu_get_opr(u_int64_t iu_id);
        u_int64_t iu_get_key(u_int64_t iu_id);
        u_int64_t iu_get_value_id(u_int64_t iu_id);
        u_int64_t iu_get_next_iu_id(u_int64_t iu_id);
        void iu_update_next_iu_id(u_int64_t iu_id, u_int64_t value);
        T iu_get_value(u_int64_t val_id);
        void iu_write_value(u_int64_t val_id, T* buf);

        u_int64_t iu_get_val_next(u_int64_t val_id);
        void iu_update_val_next(u_int64_t val_id, u_int64_t next);

        // HBTree
        bool hb_precheck(u_int64_t node_id);
        void hb_load(BTreeNode<T>* node, u_int64_t cmb_id);
        void hb_store(u_int64_t cmb_id, BTreeNode<T>* node);

        u_int64_t hb_kvp_search(u_int64_t cmb_id, u_int64_t _k);
        void hb_insert(u_int64_t cmb_id, u_int64_t _k, T _v);
        void hb_update(u_int64_t cmb_id, u_int64_t _k, T _v);
        T hb_search(u_int64_t cmb_id, u_int64_t _k);
        void hb_delete(u_int64_t cmb_id, u_int64_t _k);

        void hb_write_key(u_int64_t cmb_id, u_int64_t idx, u_int64_t _k);
        u_int64_t hb_read_key(u_int64_t cmb_id, u_int64_t idx);
        void hb_write_value(u_int64_t cmb_id, u_int64_t idx, T _v);
        T hb_read_value(u_int64_t cmb_id, u_int64_t idx);
        void hb_write_num_key(u_int64_t cmb_id, u_int64_t num_key);        
        u_int64_t hb_read_num_key(u_int64_t cmb_id);        
};

class meta_BKMap{
    public:
        u_int64_t new_node_id;
        u_int64_t clear_ptr;
        u_int64_t num_iu;
        u_int64_t free_iu_stack_id;
        u_int64_t free_iu_id;
        u_int64_t free_val_stack_id;
        u_int64_t free_val_id;
};

class BKMap{
    public:
        u_int64_t block_id;
        u_int64_t num_kv;
        u_int64_t is_leaf;
        u_int64_t iu_ptr; 
};

class LRU_list_entry{
    public:
        u_int64_t last;
        u_int64_t next;
};

class node_LRU{
    public:
        LRU_list_entry* list_pool;
        u_int64_t head; 
        u_int64_t tail;
        bool LRU;

        node_LRU();
        ~node_LRU();

        void dequeue(u_int64_t node_id);
        bool look_up(u_int64_t node_id);
        void insert(u_int64_t node_id);
        u_int64_t pop();
};

class APPEND_ENTRY{
    public:
        u_int64_t opr;
        u_int64_t key;
        u_int64_t next;
        u_int64_t value_ptr;
};

class IU_LIST{
    public:
        u_int64_t iu_id;
        IU_LIST* next;
};

class HB_LRU_list_entry{
    public:
        u_int64_t last;
        u_int64_t NODE_ID;
        u_int64_t used;
        u_int64_t next;
};

class HB_LRU{
    public:
       HB_LRU_list_entry* list_pool; 
       u_int64_t head;
       u_int64_t tail;
       u_int64_t max_num;
       u_int64_t used;
    
       HB_LRU(int NUM);
       ~HB_LRU();

       bool full();
       u_int64_t look_up(u_int64_t node_id);
       void reenqueue(u_int64_t cmb_idx);
       u_int64_t pop();
       u_int64_t enqueue(u_int64_t node_id);
       u_int64_t dequeue(u_int64_t node_id);
};

template <typename T>
BTree<T>::BTree(char* filename, int degree, MODE _mode, int append, int read_threshold, bool LRU, int HB){
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
    height = 0;
    
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
        cmb = new CMB<T>(mode);    
        cmb->new_node_id_init();
        cmb->opt = 1;

        // Append entry Optimization
        cmb->nodeLRU = NULL;
        if(append > 0){
            cmb->nodeLRU = new node_LRU;
            cmb->nodeLRU->LRU = LRU;
            cmb->MAX_NUM_IU = append;
            cmb->read_threshold = read_threshold;

            cmb->update_num_iu(0);
            cmb->set_clear_ptr(0);
            cmb->update_free_iu_stack_id(0);
            cmb->update_free_iu_id(1);
            cmb->update_free_val_stack_id(0);
            cmb->update_free_val_id(1);
            cmb->opt = 2;
        }
        cmb->hbLRU = NULL;
        if(HB){
            cmb->hbLRU = new HB_LRU(HB);
            cmb->opt = 3;
        }
    }

}

template <typename T>
BTree<T>::~BTree(){
    mylog << "~BTree()" << endl;
    close(fd);
    if(cmb) delete cmb;
}

template <typename T>
void BTree<T>::stat(){
    cout << endl;
    cout << "Degree: " << m << endl;
    cout << "fd: " << fd << endl;
    cout << "Block size: " << block_size << endl;
    cout << "Block Capacity: " << block_cap << endl;
    cout << "Root Block ID: " << root_id << endl;
    if(cmb && cmb->nodeLRU){
        cout << "Maximum number of IU: " << cmb->MAX_NUM_IU << endl;
        cout << "Read Threshold: " << cmb->read_threshold << endl;
    }
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
    cout << hex;
    cout << "BLOCK_MAPPING_START_ADDR:\t0x" << BLOCK_MAPPING_START_ADDR << endl;
    cout << "BLOCK_MAPPING_END_ADDR:\t\t0x" << BLOCK_MAPPING_END_ADDR << endl;
    cout << "APPEND_OPR_START_ADDR:\t\t0x" << APPEND_OPR_START_ADDR << endl;
    cout << "APPEND_OPR_END_ADDR:\t\t0x" << APPEND_OPR_END_ADDR << endl;
    cout << "VALUE_POOL_START_ADDR:\t\t0x" << VALUE_POOL_START_ADDR << endl;
    cout << "VALUE_POOL_END_ADDR:\t\t0x" << VALUE_POOL_END_ADDR << endl; 
    cout << "END_ADDR:\t\t\t0x" << END_ADDR << endl;
    cout << dec;
}

template <typename T>
void BTree<T>::tree_read(int fd, BTree* tree){ 
    mylog << "tree_read()" << endl;

    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);

    auto start = chrono::high_resolution_clock::now();
    pread(fd, buf, PAGE_SIZE, 1 * PAGE_SIZE);
    auto end = std::chrono::high_resolution_clock::now();

    flash_read_size += PAGE_SIZE;
    
    memcpy((void*)tree, buf, sizeof(BTree));
    free(buf);
}

template <typename T>
void BTree<T>::tree_write(int fd, BTree* tree){
    mylog << "tree_write()" << endl;

    // Read before write to maintain the same block bitmap

    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);

    auto start = chrono::high_resolution_clock::now();
    pread(fd, buf, PAGE_SIZE, 1 * PAGE_SIZE);
    auto end = std::chrono::high_resolution_clock::now();

    flash_write_size += PAGE_SIZE;
    
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
        exit(1);
    }

    delete [] node->key;
    delete [] node->value;
    delete [] node->child_id;

    u_int64_t block_id;
    if(cmb && cmb->opt < 3)
        block_id = cmb->get_block_id(node_id);       
    else
        block_id = node_id;

    if(block_id < 2){
        cout << " error : attempt to read block id = " << block_id << endl;
        mylog << " error : attempt to read block id = " << block_id << endl;
        exit(1);
    }    

    char* buf;
    posix_memalign((void**)&buf, PAGE_SIZE, PAGE_SIZE);

    auto start = chrono::high_resolution_clock::now();
    pread(fd, buf, PAGE_SIZE, block_id * PAGE_SIZE);
    auto end = std::chrono::high_resolution_clock::now();
    flash_diff += end - start;

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

    flash_read_size += PAGE_SIZE;
}

template <typename T>
void BTree<T>::node_write(u_int64_t node_id, BTreeNode<T>* node){
    mylog << "node_write() - node_id:" << node_id << endl;

    if(node_id == 0){
        cout << " Error : Attempt to write node id = 0 " << endl;
        mylog << " Error : Attempt to write node id = 0 " << endl;
        exit(1);
    }

    u_int64_t block_id;
    if(cmb && cmb->opt < 3)
        block_id = cmb->get_block_id(node_id);
    else
        block_id = node_id;

    if(block_id < 2){
        cout << " error : attempt to write block id = " << block_id << endl;
        mylog << " error : attempt to write block id = " << block_id << endl;
        exit(1);
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

    auto start = chrono::high_resolution_clock::now();
    pwrite(fd, buf, PAGE_SIZE, block_id * PAGE_SIZE);
    auto end = std::chrono::high_resolution_clock::now();
    flash_diff += end - start;

    free(buf);

    flash_write_size += PAGE_SIZE;
}

template <typename T>
u_int64_t BTree<T>::get_free_block_id(){

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
                mylog << "get_free_block_id(): " << id << endl;
                return id;
            }
            byte >>= 1;
            id++;
        }
        byte_ptr++;
    }
    free(buf);
    mylog << " error: get_free_block_id() failed " << endl;
    exit(1);
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
    else{
        ofstream outFile(filename);
        mylog << " The tree is empty! " << endl;
    }
}

template <typename T>
void BTree<T>::search(u_int64_t _k, T* buf){
    mylog << "search() - key:" << _k << endl;

    if(root_id){
        removeList* rmlist = NULL;

        if(cmb && cmb->opt == 2){
            u_int64_t getIsLeaf = cmb->get_is_leaf(root_id);
            if(getIsLeaf == 1){
                u_int64_t iu_id = cmb->search_entry(root_id, _k, this, &rmlist);
                if(iu_id != 0){
                    OPR_CODE last_opr = cmb->iu_get_opr(iu_id);
                    if(last_opr != D){
                        u_int64_t val_id = cmb->iu_get_value_id(iu_id);
                        T ret_val = cmb->iu_get_value(val_id);
                        memcpy(buf, &ret_val, sizeof(T));
                        return;
                    }
                }
            }
        }

        BTreeNode<T>* root = new BTreeNode<T>(0, 0, 0);
        node_read(root_id, root);
        root->search(this, _k, buf, &rmlist);
        delete root;

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
    }
    else
        buf = NULL;
}

template <typename T>
void BTree<T>::update(u_int64_t _k, T _v){
    mylog << "update() : update key:" << _k << endl;

    if(root_id){

        removeList* rmlist = NULL;

        bool append_path = false;
        if(cmb && cmb->opt == 2){
            u_int64_t getIsLeaf = cmb->get_is_leaf(root_id);
            if(getIsLeaf == 1){
                append_path = true;
            }
        }

        if(append_path){
            cmb->append(this, root_id, U, _k, _v, &rmlist);
        }
        else{
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
            delete root;
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
    }
}

template <typename T>
void BTree<T>::insertion(u_int64_t _k, T _v){
    mylog << "insertion() - key:" << _k << endl;

    needsplit = false;
    hb_block = false;

    if(root_id){
        removeList* rmlist = NULL;

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

        if(needsplit){
            height++;
            cout << endl << "Tree height => " << height << endl;

            int new_root_id;     

            if(cmb && cmb->opt < 3){
                new_root_id = cmb->get_new_node_id();
                if(cmb->opt == 2)
                    cmb->update_iu_ptr(new_root_id, 0);  
                cmb->update_node_id(new_root_id, get_free_block_id());
                cmb->update_num_kv(new_root_id, 0);
                if(cmb && cmb->opt == 2){
                    cmb->update_is_leaf(new_root_id, 0);
                }
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
        // Create New Root for the first time
        if(cmb && cmb->opt < 3){
            root_id = cmb->get_new_node_id();
            if(cmb->opt == 2)
                cmb->update_iu_ptr(root_id, 0);
            cmb->update_node_id(root_id, get_free_block_id());
            cmb->update_num_kv(root_id, 0);
            if(cmb && cmb->opt == 2){
                cmb->update_is_leaf(root_id, 1);
            }
        }
        else
            root_id = get_free_block_id();

        tree_write(fd, this);

        BTreeNode<T>* root = new BTreeNode<T>(m, true, root_id);
        node_write(root_id, root);
        delete root;
        
        height++;
        cout << endl << "Tree height => " << height << endl;

        insertion(_k, _v);
    }
}

template <typename T>
void BTree<T>::deletion(u_int64_t _k){
    mylog << "deletion() - key:" << _k << endl;

    if(root_id){
        removeList* rmlist = NULL;
        hb_block = false;

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
            height--;
            cout << endl << "Tree height => " << height << endl;

            if(cmb && cmb->opt < 3)
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
void BTree<T>::hb_force_merge(u_int64_t _k, u_int64_t cmb_id){
    mylog << "hb_force_merge() - key: " << _k << ", cmb_id: " << cmb_id << endl;

    if(root_id){
        removeList* rmlist = NULL;
        
        if(cmb && cmb->opt == 3){
            BTreeNode<T>* root = new BTreeNode<T>(0,0,0);
            node_read(root_id, root);
            int dup_node_id = root->hb_force_merge(this, _k, cmb_id, &rmlist);

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
        }
    }
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
    if(t->cmb && t->cmb->opt == 2 && is_leaf)
        t->cmb->reduction(node_id, node);

    int i = 0;
    for(i = 0; i < node->num_key; i++){
        if(!is_leaf){
            BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
            t->node_read(node->child_id[i], child);
            child->display_tree(t, mode, level + 1);
            delete child;
        }
        for(int j = 0; j < level; j++) cout << '\t';
        cout << node->key[i] << endl;;
    }
    if(!is_leaf){
        BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
        t->node_read(node->child_id[i], child);
        child->display_tree(t, mode, level + 1);
        delete child;
    }
}

template <typename T>
void BTreeNode<T>::inorder_traversal(BTree<T>* t, ofstream &outFile){
    mylog << "inorder_traversal()" << endl;

    BTreeNode<T>* node = new BTreeNode<T>(0,0,0);
    t->node_read(node_id, node);

    if(t->cmb && t->cmb->opt == 2 && is_leaf){
        t->cmb->reduction(node_id, node);
    }
    if(t->cmb && t->cmb->opt == 3 && is_leaf){
        u_int64_t cmb_id = t->cmb->hbLRU->look_up(node_id);
        if(cmb_id < t->cmb->hbLRU->max_num){
            t->cmb->hb_store(cmb_id, node);
        } 
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
void BTreeNode<T>::search(BTree<T>* t, u_int64_t _k, T* buf, removeList** list){
    mylog << "search() - key:" << _k << endl;

    if(is_leaf && t->cmb && t->cmb->opt == 3){
        u_int64_t cmb_id = t->cmb->hbLRU->look_up(node_id);
        if(cmb_id < t->cmb->hbLRU->max_num){
            t->cmb->hbLRU->reenqueue(cmb_id);
        } 
        else{
            if(t->cmb->hbLRU->full()){
                u_int64_t cmb_id = t->cmb->hbLRU->pop();
                u_int64_t first_key = t->cmb->hb_read_key(cmb_id, 0);
                t->hb_force_merge(first_key, cmb_id);
                t->search(_k, buf);
                return;
            }
            else{
                cmb_id = t->cmb->hbLRU->enqueue(node_id);
                t->cmb->hb_load(this, cmb_id);
            }
        }
        T ret_val = t->cmb->hb_search(cmb_id, _k);
        memcpy(buf, &ret_val, sizeof(T));
        return;
    }

    int i;
    for(i = 0; i < num_key; i++){
        if(_k == key[i]){

            // Key match
            memcpy(buf, &value[i], sizeof(T));

            return;
        }
        if(_k < key[i]){ 
            if(!is_leaf){
                if(t->cmb && t->cmb->opt == 2){
                    u_int64_t getIsLeaf = t->cmb->get_is_leaf(child_id[i]);
                    if(getIsLeaf == 1){
                        u_int64_t iu_id = t->cmb->search_entry(child_id[i], _k, t, list);
                        if(iu_id != 0){
                            OPR_CODE last_opr = t->cmb->iu_get_opr(iu_id);
                            if(last_opr != D){
                                u_int64_t val_id = t->cmb->iu_get_value_id(iu_id);
                                T ret_val = t->cmb->iu_get_value(val_id);
                                memcpy(buf, &ret_val, sizeof(T));
                                return;
                            }
                        }
                    }
                }

                BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
                t->node_read(child_id[i], child);
                child->search(t, _k, buf, list);
                delete child;
            }
            else
                buf = NULL;

            return;
        }
    }

    if(!is_leaf){
        if(t->cmb && t->cmb->opt == 2){
            u_int64_t getIsLeaf = t->cmb->get_is_leaf(child_id[i]);
            if(getIsLeaf == 1){
                u_int64_t iu_id = t->cmb->search_entry(child_id[i], _k, t, list);
                if(iu_id != 0){
                    OPR_CODE last_opr = t->cmb->iu_get_opr(iu_id);
                    if(last_opr != D){
                        u_int64_t val_id = t->cmb->iu_get_value_id(iu_id);
                        T ret_val = t->cmb->iu_get_value(val_id);
                        memcpy(buf, &ret_val, sizeof(T));
                        return;
                    }
                }
            }
        }

        BTreeNode<T>* child = new BTreeNode<T>(0, 0, 0);
        t->node_read(child_id[i], child);
        child->search(t, _k, buf, list);
        delete child;
    }
    else
        buf = NULL;
}

template <typename T>
u_int64_t BTreeNode<T>::update(BTree<T>* t, u_int64_t _k, T _v, removeList** list){
    mylog << "update() - key:" << _k << endl;

    if(is_leaf && t->cmb && t->cmb->opt == 3){
        u_int64_t cmb_id = t->cmb->hbLRU->look_up(node_id);
        if(cmb_id < t->cmb->hbLRU->max_num){
            t->cmb->hbLRU->reenqueue(cmb_id);
        } 
        else{
            if(t->cmb->hbLRU->full()){
                u_int64_t cmb_id = t->cmb->hbLRU->pop();
                u_int64_t first_key = t->cmb->hb_read_key(cmb_id, 0);
                t->hb_force_merge(first_key, cmb_id);
                t->update(_k, _v);
                return 0;
            }
            else{
                cmb_id = t->cmb->hbLRU->enqueue(node_id);
                t->cmb->hb_load(this, cmb_id);
            }
        }
        t->cmb->hb_update(cmb_id, _k, _v);
        return node_id;
    }

    int i;
    for(i = 0; i < num_key; i++){
        // Key match
        if(_k == key[i]){
            if(t->cmb && t->cmb->opt == 2 && is_leaf){
                t->cmb->append(t, node_id, U, _k, _v, list);
            }
            else{
                value[i] = _v;

                if(t->cmb && t->cmb->opt < 3){
                    *list = new removeList(t->cmb->get_block_id(node_id), *list);
                    t->cmb->update_node_id(node_id, t->get_free_block_id());
                }
                else{
                    *list = new removeList(node_id, *list);
                    node_id = t->get_free_block_id();
                }                

                t->node_write(node_id, this);
            }

            return node_id;
        }
        if(_k < key[i])
            break;        
    }

    if(is_leaf)
        return 0; // Not found
    else{
        if(t->cmb && t->cmb->opt == 2){
            u_int64_t getIsLeaf = t->cmb->get_is_leaf(child_id[i]); 
            if(getIsLeaf == 1){
                t->cmb->append(t, child_id[i], U, _k, _v, list);
                return node_id;                
            }
        }

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

        if(needsplit){
            node_id = split(t, child_id[i], node_id, list);
        }
        else if(t->cmb && t->cmb->opt == 2 && child->is_leaf){           
            if(t->cmb->get_num_kv(child_id[i]) >= m)
                node_id = split(t, child_id[i], node_id, list);
        }
            

        delete child;

        return node_id;
    }
}

template <typename T>
u_int64_t BTreeNode<T>::direct_insert(BTree<T>* t, u_int64_t _k, T _v, removeList** list, u_int64_t node_id1, u_int64_t node_id2){
    /* Assume the list is not full */
    if(num_key >= m) return node_id;

    mylog << "direct_insert() - key:" << _k << endl;

    if(is_leaf && t->cmb && t->cmb->opt == 3 && !hb_block){
        u_int64_t cmb_id = t->cmb->hbLRU->look_up(node_id);
        if(cmb_id < t->cmb->hbLRU->max_num){
            if(t->cmb->hb_read_num_key(cmb_id) >= m-1){
                t->cmb->hbLRU->dequeue(node_id);
                t->cmb->hb_store(cmb_id, this);
            }
            else{
                t->cmb->hbLRU->reenqueue(cmb_id);
                t->cmb->hb_insert(cmb_id, _k, _v);
                return node_id;
            }
        }
        else{
            if(num_key < m-1){
                if(t->cmb->hbLRU->full()){
                    u_int64_t cmb_id = t->cmb->hbLRU->pop();
                    u_int64_t first_key = t->cmb->hb_read_key(cmb_id, 0);
                    t->hb_force_merge(first_key, cmb_id);
                    t->insertion(_k, _v);
                    return 0;
                }
                else{
                    cmb_id = t->cmb->hbLRU->enqueue(node_id);
                    t->cmb->hb_load(this, cmb_id);
                    t->cmb->hb_insert(cmb_id, _k, _v);
                    return node_id;
                }
            }
        }
    }

    if(t->cmb && t->cmb->opt == 2 && is_leaf){
        t->cmb->append(t, node_id, I, _k, _v, list);

        u_int64_t num_k = t->cmb->get_num_kv(node_id) + 1;
        t->cmb->update_num_kv(node_id, num_k);
        if(num_k >= m) needsplit = true;
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
        if(num_key >= m) needsplit = true;

        if(t->cmb && t->cmb->opt < 3){
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(node_id, t->get_free_block_id());
            t->cmb->update_num_kv(node_id, num_key);
        }
        else{
            *list = new removeList(node_id, *list);
            node_id = t->get_free_block_id();
        }

        t->node_write(node_id, this);
    }

    return node_id;
}

template <typename T>
u_int64_t BTreeNode<T>::split(BTree<T>*t, u_int64_t spt_node_id, u_int64_t parent_id, removeList** list){
    mylog << "split() - node id:" << spt_node_id << " parent node id:" << parent_id << endl;

    needsplit = false;
    structural_change = 'T';
    
    BTreeNode<T>* node = new BTreeNode<T>(0, 0, 0);
    t->node_read(spt_node_id, node);
    if(t->cmb && t->cmb->opt == 2 && node->is_leaf){
        t->cmb->reduction(spt_node_id, node);
    }

    BTreeNode<T>* parent = new BTreeNode<T>(0, 0, 0);
    t->node_read(parent_id, parent);  

    int new_node_id;
    if(t->cmb && t->cmb->opt < 3){
        new_node_id = t->cmb->get_new_node_id();
        if(t->cmb->opt == 2)
            t->cmb->update_iu_ptr(new_node_id, 0);
        t->cmb->update_node_id(new_node_id, t->get_free_block_id());
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

    if(t->cmb && t->cmb->opt < 3){
        *list = new removeList(t->cmb->get_block_id(node->node_id), *list);
        
        t->cmb->update_num_kv(new_node_id, new_node->num_key);

        t->cmb->update_node_id(node->node_id, t->get_free_block_id());
        t->cmb->update_num_kv(node->node_id, min_num);
        
        if(t->cmb && t->cmb->opt == 2){
            t->cmb->clear_iu(node->node_id); 
            if(node->is_leaf)
                t->cmb->update_is_leaf(new_node_id, 1);
            else
                t->cmb->update_is_leaf(new_node_id, 0);
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

    if(t->cmb && t->cmb->opt == 2 && is_leaf){
        t->cmb->reduction(node_id, this);
    }
    if(t->cmb && t->cmb->opt == 3 && is_leaf){
        u_int64_t cmb_id = t->cmb->hbLRU->look_up(node_id);
        if(cmb_id < t->cmb->hbLRU->max_num){
            t->cmb->hb_store(cmb_id, this); 
        }
    }

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

            bool borrow_from_succ = false;
            if(t->cmb && t->cmb->opt == 2){
                if(t->cmb->get_num_kv(succ_id) > min_num)
                    borrow_from_succ = true;
            }                
            else if(t->cmb && t->cmb->opt == 3){
                u_int64_t cmb_id = t->cmb->hbLRU->look_up(succ_id);
                if(cmb_id < t->cmb->hbLRU->max_num){
                    if(t->cmb->hb_read_num_key(cmb_id) > min_num){
                        borrow_from_succ = true;
                    }
                }
                else{
                    if(succ->num_key > min_num)
                        borrow_from_succ = true;
                }
            }
            else{
                if(succ->num_key > min_num)
                    borrow_from_succ = true;
            }

            if(borrow_from_succ){
                mylog << "borrow_from_succesor() - node id:" << node->node_id << endl;
                if(t->cmb && t->cmb->opt == 2)
                    t->cmb->reduction(succ_id, succ);
                if(t->cmb && t->cmb->opt == 3){
                    u_int64_t cmb_id = t->cmb->hbLRU->look_up(succ_id);
                    if(cmb_id < t->cmb->hbLRU->max_num){
                        t->cmb->hbLRU->dequeue(succ_id);
                        u_int64_t first_key = t->cmb->hb_read_key(cmb_id, 0); 
                        t->hb_force_merge(first_key, cmb_id);
                        t->deletion(_k);
                        return 0;
                    } 
                }
                
                hb_block = true;                 
                // Borrow from succ
                key[i] = succ->key[0];
                value[i] = succ->value[0];
                // Delete the kv from succ
                child_id[i+1] = node->traverse_delete(t, key[i], list);

                if(t->cmb && t->cmb->opt < 3){
                    *list = new removeList(t->cmb->get_block_id(node_id), *list);
                    t->cmb->update_node_id(node_id, t->get_free_block_id());
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
                if(t->cmb && t->cmb->opt == 2)
                    t->cmb->reduction(pred_id, pred);
                if(t->cmb && t->cmb->opt == 3){
                    u_int64_t cmb_id = t->cmb->hbLRU->look_up(pred_id);
                    if(cmb_id < t->cmb->hbLRU->max_num){
                        t->cmb->hbLRU->dequeue(pred_id);
                        u_int64_t first_key = t->cmb->hb_read_key(cmb_id, 0); 
                        t->hb_force_merge(first_key, cmb_id);
                        t->deletion(_k);
                        return 0;
                    } 
                }

                mylog << "borrow_from_predecesor() - node id:" << node->node_id << endl;

                hb_block = true;
                // Borrow from pred
                key[i] = pred->key[pred->num_key - 1];
                value[i] = pred->value[pred->num_key - 1];
                // Delete the kv form pred
                child_id[i] = node->traverse_delete(t, key[i], list);

                if(t->cmb && t->cmb->opt < 3){
                    *list = new removeList(t->cmb->get_block_id(node_id), *list);
                    t->cmb->update_node_id(node_id, t->get_free_block_id());
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
u_int64_t BTreeNode<T>::direct_delete(BTree<T>* t, u_int64_t _k, removeList** list){
    mylog << "direct_delete() - key:" << _k << endl;

    if(is_leaf && t->cmb && t->cmb->opt == 3 && !hb_block){
        u_int64_t cmb_id = t->cmb->hbLRU->look_up(node_id);
        if(cmb_id < t->cmb->hbLRU->max_num){
            if(t->cmb->hb_read_num_key(cmb_id) <= min_num){
                t->cmb->hbLRU->dequeue(node_id);
                t->cmb->hb_store(cmb_id, this);
            }
            else{
                t->cmb->hbLRU->reenqueue(cmb_id);
                t->cmb->hb_delete(cmb_id, _k);
                return node_id;
            }
        }
        else{
            if(num_key > min_num){
                if(t->cmb->hbLRU->full()){
                    u_int64_t cmb_id = t->cmb->hbLRU->pop();
                    u_int64_t first_key = t->cmb->hb_read_key(cmb_id, 0);
                    t->hb_force_merge(first_key, cmb_id);
                    t->deletion(_k);
                    return 0;
                }
                else{
                    cmb_id = t->cmb->hbLRU->enqueue(node_id);
                    t->cmb->hb_load(this, cmb_id);
                    t->cmb->hb_delete(cmb_id, _k);
                    return node_id;
                }
            }
        }
    }

    int i, idx;
    for(i = 0; i < num_key; i++){
        if(key[i] == _k) break;
    }
    idx = i;
    
    if(i == num_key){
        mylog << "~~Key not found~~" << endl;

        return 0;
    } 

    if(t->cmb && (i < num_key && t->cmb->opt == 2 && is_leaf)){
        T empty;
        t->cmb->append(t, node_id, D, _k, empty, list);
        t->cmb->update_num_kv(node_id, t->cmb->get_num_kv(node_id) - 1);
    }
    else{
        if(i < num_key-1){
            for(; i < num_key-1; i++){
                key[i] = key[i+1];
                value[i] = value[i+1];
                if(!is_leaf) child_id[i] = child_id[i+1];
            }
            if(!is_leaf) child_id[i] = child_id[i+1];
        }
        num_key--;

        if(t->cmb && t->cmb->opt < 3){
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(node_id, t->get_free_block_id());
            t->cmb->update_num_kv(node_id, num_key);
        }
        else{
            *list = new removeList(node_id, *list);
            node_id = t->get_free_block_id();
        }

        t->node_write(node_id, this);
    }

    return node_id;
}

template <typename T>
u_int64_t BTreeNode<T>::rebalance(BTree<T>* t, int idx, removeList** list){

    BTreeNode<T>* node = new BTreeNode<T>(0, 0, 0);
    t->node_read(child_id[idx], node);

    if(t->cmb && t->cmb->opt == 2 && node->is_leaf){
        if(t->cmb->get_num_kv(child_id[idx]) >= min_num){
            delete node;
            return node_id;
        }
    }
    else if(t->cmb && t->cmb->opt == 3 && node->is_leaf){
        u_int64_t cmb_id = t->cmb->hbLRU->look_up(node->node_id);
        if(cmb_id < t->cmb->hbLRU->max_num){
            if(t->cmb->hb_read_num_key(node->node_id) >= min_num){
                delete node;
                return node_id; 
            }
        }
        else{
            if(node->num_key >= min_num){
                delete node;
                return node_id;
            }
        }
    }
    else if(node->num_key >= min_num){
        delete node;
        return node_id;
    }

    mylog << "rebalance() - node id:" << child_id[idx] << endl;
    structural_change = 'T';

    BTreeNode<T>* left = new BTreeNode<T>(0, 0, 0);
    if(idx - 1 >= 0){
        t->node_read(child_id[idx - 1], left);
         
        if(t->cmb && t->cmb->opt == 3 && left->is_leaf){
            u_int64_t cmb_id = t->cmb->hbLRU->look_up(left->node_id);
            if(cmb_id < t->cmb->hbLRU->max_num){
                t->cmb->hb_store(cmb_id, left);      
            }
        }
    }
    BTreeNode<T>* right = new BTreeNode<T>(0, 0, 0);
    if(idx + 1 <= num_key){
        t->node_read(child_id[idx + 1], right);

        if(t->cmb && t->cmb->opt == 3 && right->is_leaf){
            u_int64_t cmb_id = t->cmb->hbLRU->look_up(right->node_id);
            if(cmb_id < t->cmb->hbLRU->max_num){
                t->cmb->hb_store(cmb_id, right);      
            }
        }
    }
    int trans_node_id;

    bool child_is_leaf = (idx - 1 >= 0) ? left->is_leaf : right->is_leaf;
    if(t->cmb && t->cmb->opt == 2 && child_is_leaf){
        if(idx - 1 >= 0 && t->cmb->get_num_kv(left->node_id) > min_num){
            mylog << "borrow_from_left_sibling() - node id:" << child_id[idx] << endl;
            t->node_read(child_id[idx-1], left);   
            t->cmb->reduction(left->node_id, left);

            // Append insert to right
            t->cmb->append(t, child_id[idx], I, key[idx-1], value[idx-1], list);
            t->cmb->update_num_kv(child_id[idx], t->cmb->get_num_kv(child_id[idx]) + 1);

            // Borrow from left to parent
            key[idx-1] = left->key[left->num_key - 1];
            value[idx-1] = left->value[left->num_key - 1];
            
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(node_id, t->get_free_block_id());

            t->node_write(node_id, this);   

            // Append delete to left
            T empty;
            t->cmb->append(t, left->node_id, D, left->key[left->num_key-1], empty, list);            

            left->num_key -= 1;
            t->cmb->update_num_kv(left->node_id, left->num_key);
        }
        else if(idx + 1 <= num_key && t->cmb->get_num_kv(right->node_id) > min_num){
            mylog << "borrow_from_right_sibling() - node id:" << child_id[idx] << endl;
            t->node_read(child_id[idx+1], right);
            t->cmb->reduction(right->node_id, right);

            // Append insert to left
            t->cmb->append(t, child_id[idx], I, key[idx], value[idx], list);
            t->cmb->update_num_kv(child_id[idx], t->cmb->get_num_kv(child_id[idx]) + 1);

            // Borrow from right to parent
            key[idx] = right->key[0];
            value[idx] = right->value[0];
            
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(node_id, t->get_free_block_id());

            t->node_write(node_id, this);  

            // Append delete to right
            T empty;
            t->cmb->append(t, right->node_id, D, right->key[0], empty, list);            

            right->num_key -= 1;
            t->cmb->update_num_kv(right->node_id, right->num_key);
        }
        else{
            mylog << "merge() - node id:" << child_id[idx] << endl;
            if(idx == t->cmb->get_num_kv(node_id)) idx -= 1;
            t->node_read(child_id[idx], left);
            t->cmb->reduction(left->node_id, left);
            t->node_read(child_id[idx+1], right);
            t->cmb->reduction(right->node_id, right);
                // Insert parent's kv to left
            left->key[left->num_key] = key[idx];
            left->value[left->num_key] = value[idx];
            left->num_key++;
                // Insert right to left
            for(int i = 0; i < right->num_key; i++){
                left->key[left->num_key] = right->key[i];
                left->value[left->num_key] = right->value[i];
                if(!right->is_leaf) 
                    left->child_id[left->num_key+1] = right->child_id[i+1];
                left->num_key++;  
            }

            *list = new removeList(t->cmb->get_block_id(left->node_id), *list);
            t->cmb->update_node_id(left->node_id, t->get_free_block_id());
            t->cmb->update_num_kv(left->node_id, left->num_key);
            t->cmb->clear_iu(left->node_id); 
            t->node_write(left->node_id, left);

            node_id = direct_delete(t, key[idx], list);
            child_id[idx] = left->node_id;
            *list = new removeList(t->cmb->get_block_id(node_id), *list);
            t->cmb->update_node_id(node_id, t->get_free_block_id());
            t->node_write(node_id, this);
            
            *list = new removeList(t->cmb->get_block_id(right->node_id), *list);
            t->cmb->clear_iu(right->node_id);

        }
    }
    else{
        if(idx - 1 >= 0 && left->num_key > min_num){
            mylog << "borrow_from_left_sibling() - node id:" << child_id[idx] << endl;
            //Borrowing from left
            t->node_read(child_id[idx-1], left);        
            if(t->cmb && t->cmb->opt == 3 && left->is_leaf){
                u_int64_t cmb_id = t->cmb->hbLRU->look_up(left->node_id);
                if(cmb_id < t->cmb->hbLRU->max_num){
                    t->cmb->hbLRU->dequeue(left->node_id); 
                    t->cmb->hb_store(cmb_id, left);      
                }
                hb_block = true;
            }
            t->node_read(child_id[idx], right);
            if(t->cmb && t->cmb->opt == 3 && right->is_leaf){
                u_int64_t cmb_id = t->cmb->hbLRU->look_up(right->node_id);
                if(cmb_id < t->cmb->hbLRU->max_num){
                    t->cmb->hbLRU->dequeue(right->node_id);
                    t->cmb->hb_store(cmb_id, right);      
                }
                hb_block = true;
            }
            trans_node_id = (left->is_leaf) ? 0 : left->child_id[left->num_key];
                // Insert to right
            child_id[idx] = right->direct_insert(t, key[idx-1], value[idx-1], list, trans_node_id);
                // Borrow from left
            key[idx-1] = left->key[left->num_key - 1];
            value[idx-1] = left->value[left->num_key - 1];
                // Delete from left
            child_id[idx-1] = left->direct_delete(t, key[idx-1], list);

            if(t->cmb && t->cmb->opt < 3){
                *list = new removeList(t->cmb->get_block_id(node_id), *list);
                t->cmb->update_node_id(node_id, t->get_free_block_id());
            }
            else{
                *list = new removeList(node_id, *list);
                node_id = t->get_free_block_id();
            }

            t->node_write(node_id, this);
        }
        else if(idx + 1 <= num_key && right->num_key > min_num){
            mylog << "borrow_from_right_sibling() - node id:" << child_id[idx] << endl;
            //Borrowing from right
            t->node_read(child_id[idx], left);
            if(t->cmb && t->cmb->opt == 3 && left->is_leaf){
                u_int64_t cmb_id = t->cmb->hbLRU->look_up(left->node_id);
                if(cmb_id < t->cmb->hbLRU->max_num){
                    t->cmb->hbLRU->dequeue(left->node_id); 
                    t->cmb->hb_store(cmb_id, left);      
                }
                hb_block = true;
            }
            t->node_read(child_id[idx+1], right);
            if(t->cmb && t->cmb->opt == 3 && right->is_leaf){
                u_int64_t cmb_id = t->cmb->hbLRU->look_up(right->node_id);
                if(cmb_id < t->cmb->hbLRU->max_num){
                    t->cmb->hbLRU->dequeue(right->node_id);
                    t->cmb->hb_store(cmb_id, right);      
                }
                hb_block = true;
            }
            trans_node_id = (right->is_leaf) ? 0 : right->child_id[0];
                // Insert to left
            child_id[idx] = left->direct_insert(t, key[idx], value[idx], list, 0, trans_node_id);
                // Borrow from right
            key[idx] = right->key[0];
            value[idx] = right->value[0];
                // Delete from left
            child_id[idx+1] = right->direct_delete(t, key[idx], list);
            
            if(t->cmb && t->cmb->opt < 3){
                *list = new removeList(t->cmb->get_block_id(node_id), *list);
                t->cmb->update_node_id(node_id, t->get_free_block_id());
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
            if(t->cmb && t->cmb->opt == 3 && left->is_leaf){
                u_int64_t cmb_id = t->cmb->hbLRU->look_up(left->node_id);
                if(cmb_id < t->cmb->hbLRU->max_num){
                    t->cmb->hbLRU->dequeue(left->node_id);
                    t->cmb->hb_store(cmb_id, left);      
                }
                hb_block = true;
            }
            t->node_read(child_id[idx+1], right);
            if(t->cmb && t->cmb->opt == 3 && right->is_leaf){
                u_int64_t cmb_id = t->cmb->hbLRU->look_up(right->node_id);
                if(cmb_id < t->cmb->hbLRU->max_num){
                    t->cmb->hbLRU->dequeue(right->node_id);
                    t->cmb->hb_store(cmb_id, right);      
                }
                hb_block = true;
            }
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
            if(t->cmb && t->cmb->opt < 3){
                *list = new removeList(t->cmb->get_block_id(left->node_id), *list);
                t->cmb->update_node_id(left->node_id, t->get_free_block_id());
                t->cmb->update_num_kv(left->node_id, left->num_key);
            }
            else{
                *list = new removeList(left->node_id, *list);
                left->node_id = t->get_free_block_id();
            }
            t->node_write(left->node_id, left);
                // Delete the parent's kv
            node_id = direct_delete(t, key[idx], list);
            child_id[idx] = left->node_id;

            if(t->cmb && t->cmb->opt < 3){
                *list = new removeList(t->cmb->get_block_id(node_id), *list);
                t->cmb->update_node_id(node_id, t->get_free_block_id());
            }
            else{
                *list = new removeList(node_id, *list);
                node_id = t->get_free_block_id();
            }

            t->node_write(node_id, this);

            if(t->cmb && t->cmb->opt < 3)
                *list = new removeList(t->cmb->get_block_id(right->node_id), *list);
            else
                *list = new removeList(right->node_id, *list);
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

template <typename T>
u_int64_t BTreeNode<T>::hb_force_merge(BTree<T> *t, u_int64_t _k, u_int64_t cmb_id, removeList** list){
    mylog << "hb_force_merge() - key: " << _k << ", cmb_id: " << cmb_id << endl;

    if(is_leaf){
        t->cmb->hb_store(cmb_id, this);  
        
        *list = new removeList(node_id, *list);
        node_id = t->get_free_block_id();
        
        t->node_write(node_id, this);
  
        return node_id;
    }
    else{
        int i;
        for(i = 0; i < num_key; i++){
            if(_k < key[i]){
                BTreeNode<T>* child = new BTreeNode<T>(0,0,0);
                t->node_read(child_id[i], child);
                int dup_child_id = child->hb_force_merge(t, _k, cmb_id, list);

                if(dup_child_id == 0){
                    delete child;
                    return 0;
                }

                if(dup_child_id != child_id[i]){
                    child_id[i] = dup_child_id;
                    *list = new removeList(node_id , *list);
                    node_id = t->get_free_block_id();
                    t->node_write(node_id, this);
                } 

                delete child;

                return node_id;
            }
        }

        BTreeNode<T>* child = new BTreeNode<T>(0,0,0);
        t->node_read(child_id[i], child);
        int dup_child_id = child->hb_force_merge(t, _k, cmb_id, list);

        if(dup_child_id == 0){
            delete child;
            return 0;
        }

        if(dup_child_id != child_id[i]){
            child_id[i] = dup_child_id;
            *list = new removeList(node_id , *list);
            node_id = t->get_free_block_id();
            t->node_write(node_id, this);
        } 

        delete child;

        return node_id;
    }
}

removeList::removeList(int _id, removeList* _next){
    id = _id;
    next = _next;
}

template <typename T>
CMB<T>::CMB(MODE _mode){
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

template <typename T>
CMB<T>::~CMB(){
    mylog << "~CMB()" << endl;

    if(mode == DRAM){
        memcpy(map_base, cache_base, MAP_SIZE);    
        free(cache_base);
    }

    if (munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);

    if(nodeLRU)
        delete nodeLRU;

    if(hbLRU)
        delete hbLRU;
}

template <typename T>
void* CMB<T>::get_map_base(){
    return map_base;
}

template <typename T>
void CMB<T>::cmb_memcpy(void* dest, void* src, size_t len){

    u_int64_t* d = (u_int64_t*) dest;
    u_int64_t* s = (u_int64_t*) src;

    auto start = chrono::high_resolution_clock::now();
    for(int i = 0; i < len / sizeof(u_int64_t); i++)
        *d++ = *s++;
    auto end = std::chrono::high_resolution_clock::now();
    cmb_diff += end - start;    
}

template <typename T>
void CMB<T>::remap(off_t offset){    
    if( ((bar_addr + offset) & ~MAP_MASK) != map_idx ){
         mylog << "remap() - offset:" << offset << " map_idx : " << map_idx << "->" << ((bar_addr + offset) & ~MAP_MASK) << endl;
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

template <typename T>
void CMB<T>::read(void* buf, off_t offset, int size){
    //mylog << "CMB.read() - offset:" << offset << ", size = " << size << endl;

    int remain_size = size;
    off_t cur_offset = offset;

    while(remain_size){
        int cp_size = remain_size;

        if(((bar_addr + cur_offset) & ~MAP_MASK) < ((bar_addr + cur_offset + remain_size - 1) & ~MAP_MASK)){
            cp_size = (((bar_addr + cur_offset) & ~MAP_MASK) + MAP_SIZE) - (bar_addr + cur_offset); 
        }

        remap(cur_offset);

        void* virt_addr;
        if(cache_base)
            virt_addr = (char*)cache_base + ((cur_offset) & MAP_MASK);	
        else
            virt_addr = (char*)map_base + ((cur_offset) & MAP_MASK);

        cmb_memcpy(((char*) buf + cur_offset - offset), virt_addr, cp_size);

        remain_size -= cp_size;
        cur_offset += cp_size;
    }

    cmb_read_size += size;
}

template <typename T>
void CMB<T>::write(off_t offset, void* buf, int size){
    //mylog << "CMB.write() - offset:" << offset << ", size = " << size << endl;

    int remain_size = size;
    off_t cur_offset = offset;

    while(remain_size){
        int cp_size = remain_size;

        if(((bar_addr + cur_offset) & ~MAP_MASK) < ((bar_addr + cur_offset + remain_size - 1) & ~MAP_MASK)){
            cp_size = (((bar_addr + cur_offset) & ~MAP_MASK) + MAP_SIZE) - (bar_addr + cur_offset); 
        }

        remap(cur_offset);

        void* virt_addr;
        if(cache_base)
            virt_addr = (char*)cache_base + ((cur_offset) & MAP_MASK);	
        else
            virt_addr = (char*)map_base + ((cur_offset) & MAP_MASK);

        cmb_memcpy(virt_addr, ((char*) buf + cur_offset - offset), cp_size);

        remain_size -= cp_size;
        cur_offset += cp_size;
    }

    cmb_write_size += size;
}

template <typename T>
void CMB<T>::addr_bkmap_check(off_t addr){
    if(addr > BLOCK_MAPPING_END_ADDR){
        cout << "CMB Block Mapping Access Out of Range" << endl;
        mylog << "CMB Block Mapping Access Out of Range" << endl;
        exit(1);
    }
}

template <typename T>
void CMB<T>::addr_bkmap_meta_check(off_t addr){
    if(addr >= BLOCK_MAPPING_START_ADDR){
        cout << "CMB Block Mapping Meta Access Out of Range" << endl;
        mylog << "CMB Block Mapping Meta Access Out of Range" << endl;
        exit(1);
    }
}

template <typename T>
u_int64_t CMB<T>::get_new_node_id(){
    mylog << "get_new_node_id()" << endl;

    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.new_node_id - (char*)&ref);
    addr_bkmap_meta_check(addr);
    u_int64_t new_node_id;
    read(&new_node_id, addr, sizeof(u_int64_t));

    if(new_node_id > (BLOCK_MAPPING_END_ADDR - BLOCK_MAPPING_START_ADDR) / sizeof(u_int64_t)){
        cout << "CMB Block Mapping Limit Exceeded" << endl;
        mylog << "CMB Block Mapping Limit Exceeded" << endl;
        exit(1);
    }
    u_int64_t next_node_id = new_node_id + 1;
    write(addr, &next_node_id, sizeof(u_int64_t));

    return new_node_id;
}

template <typename T>
void CMB<T>::new_node_id_init(void){
    mylog << "new_node_id_init() - 1" << endl;

    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.new_node_id - (char*)&ref);
    addr_bkmap_meta_check(addr);

    u_int64_t new_node_id = 1;
    write(addr, &new_node_id, sizeof(u_int64_t));
}

template <typename T>
u_int64_t CMB<T>::get_block_id(u_int64_t node_id){
    mylog << "get_block_id() - node id:" << node_id << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.block_id - (char*)&ref);
    addr_bkmap_check(addr);

    u_int64_t readval;
	read(&readval, addr, sizeof(u_int64_t));
    return readval;
}

template <typename T>
void CMB<T>::update_node_id(u_int64_t node_id, u_int64_t block_id){
    mylog << "update_node_id() - node id:" << node_id << " block id:" << block_id << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.block_id - (char*)&ref);
    addr_bkmap_check(addr);

    u_int64_t writeval = block_id;
	write(addr, &writeval, sizeof(u_int64_t));

}

template <typename T>
u_int64_t CMB<T>::get_num_kv(u_int64_t node_id){
    mylog << "get_num_kv() - node id:" << node_id << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.num_kv - (char*)&ref);
    addr_bkmap_check(addr);

    u_int64_t readval;
	read(&readval, addr, sizeof(u_int64_t));
    return readval;
}

template <typename T>
void CMB<T>::update_num_kv(u_int64_t node_id, u_int64_t value){
    mylog << "update_num_kv() - node id:" << node_id << " value:" << value << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.num_kv - (char*)&ref);
    addr_bkmap_check(addr);

	write(addr, &value, sizeof(u_int64_t));
}

template <typename T>
u_int64_t CMB<T>::get_is_leaf(u_int64_t node_id){
    mylog << "get_is_leaf() - node id:" << node_id << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.is_leaf - (char*)&ref);
    addr_bkmap_check(addr);

    u_int64_t readval;
	read(&readval, addr, sizeof(u_int64_t));
    return readval;
}

template <typename T>
void CMB<T>::update_is_leaf(u_int64_t node_id, u_int64_t value){
    mylog << "update_is_leaf() - node id:" << node_id << " value:" << value << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.is_leaf - (char*)&ref);
    addr_bkmap_check(addr);

	write(addr, &value, sizeof(u_int64_t));
}

template <typename T>
u_int64_t CMB<T>::get_iu_ptr(u_int64_t node_id){
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.iu_ptr - (char*)&ref);
    addr_bkmap_check(addr);

    u_int64_t readval;
	read(&readval, addr, sizeof(u_int64_t));

    mylog << "get_iu_ptr() - node id:" << node_id << " -> " << readval << endl;

    return readval;
}

template <typename T>
void CMB<T>::update_iu_ptr(u_int64_t node_id, u_int64_t value){
    mylog << "update_iu_ptr() - node id:" << node_id << " value:" << value << endl;
    BKMap ref;
    off_t addr = BLOCK_MAPPING_START_ADDR + node_id * sizeof(BKMap) + ((char*)&ref.iu_ptr - (char*)&ref);
    addr_bkmap_check(addr);

	write(addr, &value, sizeof(u_int64_t));
}

template <typename T>
u_int64_t CMB<T>::get_num_iu(){
    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.num_iu - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
    u_int64_t readval;
    read(&readval, addr, sizeof(u_int64_t));

    mylog << "get_num_iu() - value: " << readval << endl;

    return readval;
}

template <typename T>
void CMB<T>::update_num_iu(u_int64_t num){
    mylog << "update_num_iu() - value: " << num << endl;

    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.num_iu - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
	write(addr, &num, sizeof(u_int64_t));
}

template <typename T>
u_int64_t CMB<T>::get_clear_ptr(){
    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.clear_ptr - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
    u_int64_t readval;
    read(&readval, addr, sizeof(u_int64_t));

    mylog << "get_clear_ptr() - node_id: " << readval << endl;

    return readval;
}

template <typename T>
void CMB<T>::set_clear_ptr(u_int64_t node_id){
    mylog << "set_clear_ptr() - value: " << node_id << endl;

    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.clear_ptr - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
	write(addr, &node_id, sizeof(u_int64_t));
}

template <typename T>
u_int64_t CMB<T>::get_free_iu_stack_id(){
    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.free_iu_stack_id - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
    u_int64_t readval;
    read(&readval, addr, sizeof(u_int64_t));

    mylog << "get_free_iu_stack_id() - node_id: " << readval << endl;

    return readval;
}

template <typename T>
void CMB<T>::update_free_iu_stack_id(u_int64_t value){
    mylog << "update_free_iu_stack_id() - value: " << value << endl;

    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.free_iu_stack_id - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
	write(addr, &value, sizeof(u_int64_t));
}

template <typename T>
u_int64_t CMB<T>::get_free_iu_id(){
    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.free_iu_id - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
    u_int64_t readval;
    read(&readval, addr, sizeof(u_int64_t));

    mylog << "get_free_iu() - iu id: " << readval << endl;

    return readval;
}

template <typename T>
void CMB<T>::update_free_iu_id(u_int64_t value){
    mylog << "update_free_iu_id() - value: " << value << endl;

    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.free_iu_id - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
	write(addr, &value, sizeof(u_int64_t));
}

template <typename T>
u_int64_t CMB<T>::get_free_val_stack_id(){
    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.free_val_stack_id - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
    u_int64_t readval;
    read(&readval, addr, sizeof(u_int64_t));

    mylog << "get_free_val_stack_id() - val id: " << readval << endl;

    return readval;
}

template <typename T>
void CMB<T>::update_free_val_stack_id(u_int64_t value){
    mylog << "update_free_val_stack_id() - value: " << value << endl;

    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.free_val_stack_id - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
	write(addr, &value, sizeof(u_int64_t));
}

template <typename T>
u_int64_t CMB<T>::get_free_val_id(){
    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.free_val_id - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
    u_int64_t readval;
    read(&readval, addr, sizeof(u_int64_t));

    mylog << "get_free_val() - node_id: " << readval << endl;

    return readval;
}

template <typename T>
void CMB<T>::update_free_val_id(u_int64_t value){
    mylog << "update_free_val_id() - value: " << value << endl;

    meta_BKMap ref;
    off_t addr = BLOCK_MAPPING_META_ADDR + ((char*)&ref.free_val_id - (char*)&ref);
    addr_bkmap_meta_check(addr);
    
	write(addr, &value, sizeof(u_int64_t));
}

template <typename T>
u_int64_t CMB<T>::pop_iu_id(){
    u_int64_t free_iu_stack_id = get_free_iu_stack_id();
    u_int64_t ret;
    if(free_iu_stack_id == 0){
        u_int64_t free_iu_id = get_free_iu_id();
        ret = free_iu_id;
        update_free_iu_id(free_iu_id + 1); 
    }
    else{
        ret = free_iu_stack_id;
        u_int64_t new_free_iu_stack_id = iu_get_next_iu_id(free_iu_stack_id); 
        update_free_iu_stack_id(new_free_iu_stack_id); 
    }

    mylog << "pop_iu_id() - " << ret << endl;
    return ret;
}

template <typename T>
void CMB<T>::push_iu_id(u_int64_t iu_id){
    u_int64_t free_iu_stack_id = get_free_iu_stack_id();
    iu_update_next_iu_id(iu_id, free_iu_stack_id);
    update_free_iu_stack_id(iu_id);
    mylog << "push_iu_id() - " << iu_id << endl;
}

template <typename T>
u_int64_t CMB<T>::pop_val_id(){
    u_int64_t free_val_stack_id = get_free_val_stack_id();
    u_int64_t ret;
    if(free_val_stack_id == 0){
        u_int64_t free_val_id = get_free_val_id();
        ret = free_val_id;
        update_free_val_id(free_val_id + 1);
    }
    else{
        ret = free_val_stack_id;
        u_int64_t new_free_val_stack_id = iu_get_val_next(free_val_stack_id);
        update_free_val_stack_id(new_free_val_stack_id);
    }

    return ret;
}

template <typename T>
void CMB<T>::push_val_id(u_int64_t val_id){
    u_int64_t free_val_stack_id = get_free_val_stack_id();
    iu_update_val_next(val_id, free_val_stack_id);
    update_free_val_stack_id(val_id);
}

template <typename T>
void CMB<T>::append(BTree<T>* t, u_int64_t node_id, OPR_CODE OPR, u_int64_t _k, T _v, removeList** list){
    if(full()){
        u_int64_t pop_id = nodeLRU->pop();
        BTreeNode<T>* node = new BTreeNode<T>(0, 0, 0);
        t->node_read(pop_id, node);
        u_int64_t old_block_id = get_block_id(pop_id);
        reduction(pop_id, node);
        update_node_id(pop_id, t->get_free_block_id());
        t->node_write(pop_id, node);
        clear_iu(pop_id);
        delete node; 
        *list = new removeList(old_block_id, *list);
    }  

    u_int64_t num_iu = get_num_iu();
    append_entry(node_id, OPR, _k, _v);
    update_num_iu(num_iu + 1); 
    
    nodeLRU->insert(node_id);
}

template <typename T>
void CMB<T>::reduction(u_int64_t node_id, BTreeNode<T>* node){
    mylog << "reduction() - node_id = " << node_id << endl;

    IU_LIST* iu_stack = NULL;
    reduction_create_iu_list(node_id, &iu_stack);

    while(iu_stack){
        OPR_CODE opr = iu_get_opr(iu_stack->iu_id);

        if(opr == I){
            reduction_insert(node, iu_stack);
        }
        else if(opr == U){
            reduction_update(node, iu_stack);
        }
        else if(opr == D){
            reduction_delete(node, iu_stack);
        }
        else{
            cout << "Incorrect OPR: " << opr << endl;
            mylog << "Incorrect OPR: " << opr << endl;
            exit(1);
        }

        IU_LIST* del_iu_list = iu_stack;
        iu_stack = iu_stack->next;
        delete del_iu_list;
    }
}

template <typename T>
void CMB<T>::clear_iu(u_int64_t node_id){
    nodeLRU->dequeue(node_id);
    set_clear_ptr(node_id);

    u_int64_t num_iu = get_num_iu(); 
     
    u_int64_t cur_iu_id = get_iu_ptr(node_id);
    while(cur_iu_id){
        u_int64_t next_iu_id = iu_get_next_iu_id(cur_iu_id);
        u_int64_t value_id = iu_get_value_id(cur_iu_id);
        if(value_id)
            push_val_id(value_id);
        push_iu_id(cur_iu_id);

        num_iu--;
        cur_iu_id = next_iu_id;
    }

    update_iu_ptr(node_id, 0);
    update_num_iu(num_iu);
}

template <typename T>
void CMB<T>::append_entry(u_int64_t node_id, OPR_CODE OPR, u_int64_t _k, T _v){

    u_int64_t iu_id = pop_iu_id();   

    APPEND_ENTRY new_entry;
    new_entry.opr = (u_int64_t) OPR;
    new_entry.key = _k;
    new_entry.next = get_iu_ptr(node_id); 

    if(OPR == I || OPR == U)
        new_entry.value_ptr = pop_val_id(); 
    else
        new_entry.value_ptr = 0;
        
    iu_write_value(new_entry.value_ptr, &_v);
    iu_write_iu(iu_id, new_entry);

    update_iu_ptr(node_id, iu_id);  

    mylog << "append_entry() - node_id = " << node_id << ", iu_id = " << iu_id << ", next = " << new_entry.next << endl;
}

template <typename T>
bool CMB<T>::full(){
    u_int64_t num_iu = get_num_iu();
    if(num_iu >= MAX_NUM_IU){
        mylog << "Append full()" << endl;
        return true;
    }
    else
        return false;
}

template <typename T>
void CMB<T>::reduction_create_iu_list(u_int64_t node_id, IU_LIST** iu_stack){
    mylog << "reduction_create_iu_list() - ";

    u_int64_t cur_iu_id = get_iu_ptr(node_id);
    IU_LIST* last_iu_list = NULL;
    while(cur_iu_id){
        mylog << cur_iu_id << "-";
        IU_LIST* new_iu_entry = new IU_LIST;
        new_iu_entry->iu_id = cur_iu_id;
        new_iu_entry->next = last_iu_list;

        *iu_stack = new_iu_entry;

        last_iu_list = new_iu_entry;
        cur_iu_id = iu_get_next_iu_id(cur_iu_id);
    }
    
    mylog << endl;
}

template <typename T>
void CMB<T>::reduction_insert(BTreeNode<T>* node, IU_LIST* iu_stack){
    
    u_int64_t cur_iu_id = iu_stack->iu_id;
    
    u_int64_t _k = iu_get_key(cur_iu_id);
    u_int64_t value_id = iu_get_value_id(cur_iu_id);
    T _v = iu_get_value(value_id);

    int i;
    bool ins = true;
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

template <typename T>
void CMB<T>::reduction_update(BTreeNode<T>* node, IU_LIST* iu_stack){
    
    u_int64_t cur_iu_id = iu_stack->iu_id;
    
    u_int64_t _k = iu_get_key(cur_iu_id);
    u_int64_t value_id = iu_get_value_id(cur_iu_id);
    T _v = iu_get_value(value_id);

    for(int i = 0; i < node->num_key; i++){
        if(_k == node->key[i])
            node->value[i] = _v;
        if(_k < node->key[i])
            break;
    }
}

template <typename T>
void CMB<T>::reduction_delete(BTreeNode<T>* node, IU_LIST* iu_stack){

    u_int64_t cur_iu_id = iu_stack->iu_id;
    
    u_int64_t _k = iu_get_key(cur_iu_id);

    int i;
    for(i = 0; i < node->num_key; i++)
        if(node->key[i] == _k) break;

    if(i == node->num_key)
        return;
    else if(i < node->num_key - 1){
        for(; i < node->num_key-1; i++){
            node->key[i] = node->key[i+1];
            node->value[i] = node->value[i+1];
        }
    }

    node->num_key--;            
}

template <typename T>
u_int64_t CMB<T>::search_entry(u_int64_t node_id, u_int64_t _k, BTree<T>* t, removeList** list){
    mylog << "search_entry() - node_id = " << node_id << endl;

    u_int64_t cur_iu_id = get_iu_ptr(node_id);
    int counter = 0;
    while(cur_iu_id){
        if(counter >= read_threshold){
            BTreeNode<T>* node = new BTreeNode<T>(0,0,0);
            t->node_read(node_id, node);
            u_int64_t old_block_id = get_block_id(node_id);
            reduction(node_id, node);
            update_node_id(node_id, t->get_free_block_id());
            t->node_write(node_id, node);
            clear_iu(node_id); 
            delete node;
            *list = new removeList(old_block_id, *list);
            mylog << "reduction triggered" << endl;
            return 0;
        }

        u_int64_t key = iu_get_key(cur_iu_id);
        if(key == _k)
            return cur_iu_id;
        cur_iu_id = iu_get_next_iu_id(cur_iu_id);

        counter++;
    }
    return 0;
}

template <typename T>
void CMB<T>::addr_IU_check(off_t addr){
    if(addr > APPEND_OPR_END_ADDR){
        cout << "CMB IU APPEND Access Out of Range" << endl;
        mylog << "CMB IU APPEND Access Out of Range" << endl;
        exit(1);
    }
}

template <typename T>
void CMB<T>::addr_value_check(off_t addr){
    if(addr > VALUE_POOL_END_ADDR){
        cout << "CMB IU APPEND Value Pool Access Out of Range" << endl;
        mylog << "CMB IU APPEND Value Pool Access Out of Range" << endl;
        exit(1);
    }
}

template <typename T>
void CMB<T>:: iu_write_iu(u_int64_t iu_id, APPEND_ENTRY iu){
   
    off_t addr = APPEND_OPR_START_ADDR + iu_id * sizeof(APPEND_ENTRY);
    addr_IU_check(addr);
    
    write(addr, &iu, sizeof(APPEND_ENTRY)); 
}

template <typename T>
OPR_CODE CMB<T>::iu_get_opr(u_int64_t iu_id){
    
    APPEND_ENTRY ref;
    off_t addr = APPEND_OPR_START_ADDR + iu_id * sizeof(APPEND_ENTRY) + ((char*)&ref.opr - (char*)&ref);
    addr_IU_check(addr);
    
    OPR_CODE ret;
    read(&ret, addr, sizeof(u_int64_t)); 
    return ret;
}

template <typename T>
u_int64_t CMB<T>::iu_get_key(u_int64_t iu_id){
    
    APPEND_ENTRY ref;
    off_t addr = APPEND_OPR_START_ADDR + iu_id * sizeof(APPEND_ENTRY) + ((char*)&ref.key - (char*)&ref);
    addr_IU_check(addr);
    
    u_int64_t ret;
    read(&ret, addr, sizeof(u_int64_t)); 
    return ret;
}

template <typename T>
u_int64_t CMB<T>::iu_get_value_id(u_int64_t iu_id){
    
    APPEND_ENTRY ref;
    off_t addr = APPEND_OPR_START_ADDR + iu_id * sizeof(APPEND_ENTRY) + ((char*)&ref.value_ptr - (char*)&ref);
    addr_IU_check(addr);
    
    u_int64_t ret;
    read(&ret, addr, sizeof(u_int64_t)); 
    return ret;
}

template <typename T>
u_int64_t CMB<T>::iu_get_next_iu_id(u_int64_t iu_id){
    
    APPEND_ENTRY ref;
    off_t addr = APPEND_OPR_START_ADDR + iu_id * sizeof(APPEND_ENTRY) + ((char*)&ref.next - (char*)&ref);
    addr_IU_check(addr);
    
    u_int64_t ret;
    read(&ret, addr, sizeof(u_int64_t)); 
    return ret;
}

template <typename T>
void CMB<T>::iu_update_next_iu_id(u_int64_t iu_id, u_int64_t value){
    
    APPEND_ENTRY ref;
    off_t addr = APPEND_OPR_START_ADDR + iu_id * sizeof(APPEND_ENTRY) + ((char*)&ref.next - (char*)&ref);
    addr_IU_check(addr);
    
    write(addr, &value, sizeof(u_int64_t)); 
}

template <typename T>
T CMB<T>::iu_get_value(u_int64_t val_id){
    
    off_t addr = VALUE_POOL_START_ADDR + val_id * sizeof(T);
    addr_value_check(addr);
    
    T ret;
    read(&ret, addr, sizeof(T)); 
    return ret;
}

template <typename T>
void CMB<T>::iu_write_value(u_int64_t val_id, T* buf){
    
    off_t addr = VALUE_POOL_START_ADDR + val_id * sizeof(T);
    addr_value_check(addr);
    
    write(addr, buf, sizeof(T)); 
}

template <typename T>
u_int64_t CMB<T>::iu_get_val_next(u_int64_t val_id){
    
    off_t addr = VALUE_POOL_START_ADDR + val_id * sizeof(T);
    addr_value_check(addr);
    
    u_int64_t ret;
    read(&ret, addr, sizeof(u_int64_t)); 
    return ret;
}

template <typename T>
void CMB<T>::iu_update_val_next(u_int64_t val_id, u_int64_t next){
    
    off_t addr = VALUE_POOL_START_ADDR + val_id * sizeof(T);
    addr_value_check(addr);
    
    write(addr, &next, sizeof(u_int64_t)); 
}

template <typename T>
void CMB<T>::hb_load(BTreeNode<T>* node, u_int64_t cmb_id){

    for(int i = 0; i < node->num_key; i++){
        hb_write_key(cmb_id, i, node->key[i]); 
        hb_write_value(cmb_id, i, node->value[i]); 
        usleep(1);
    }
    hb_write_num_key(cmb_id, node->num_key);
}

template <typename T>
void CMB<T>::hb_store(u_int64_t cmb_id, BTreeNode<T>* node){

    if(node->node_id != hbLRU->list_pool[cmb_id].NODE_ID){
        cout << "   hb_store error: node ID unmatch" << endl;
        mylog << "  hb_store error: node ID = " << node->node_id << ", node ID in LRU = " << hbLRU->list_pool[cmb_id].NODE_ID << endl;
        exit(1);
    }

    node->num_key = hb_read_num_key(cmb_id);
    for(int i = 0; i < node->num_key; i++){
        node->key[i] = hb_read_key(cmb_id, i);
        node->value[i] = hb_read_value(cmb_id, i);
    }
}

template <typename T>
u_int64_t CMB<T>::hb_kvp_search(u_int64_t cmb_id, u_int64_t _k){
    
    int idx;
    int num_key = hb_read_num_key(cmb_id);
    for(idx = 0; idx < num_key; idx++){
        u_int64_t key = hb_read_key(cmb_id, idx);
        if(_k <= key){
            break;
        }
    } 
    return idx;
}

template <typename T>
void CMB<T>::hb_insert(u_int64_t cmb_id, u_int64_t _k, T _v){
    
    int idx = hb_kvp_search(cmb_id, _k);
    int num_key = hb_read_num_key(cmb_id);
    for(int i = num_key-1; i >= idx; i--){
        u_int64_t last_key = hb_read_key(cmb_id, i);
        T last_value = hb_read_value(cmb_id, i);
        
        hb_write_key(cmb_id, i+1, last_key);
        hb_write_value(cmb_id, i+1, last_value);
        usleep(1);
    }
    
    hb_write_key(cmb_id, idx, _k);
    hb_write_value(cmb_id, idx, _v);

    hb_write_num_key(cmb_id, num_key+1);
}

template <typename T>
void CMB<T>::hb_update(u_int64_t cmb_id, u_int64_t _k, T _v){
    
    int idx = hb_kvp_search(cmb_id, _k);
    hb_write_value(cmb_id, idx, _v);
}

template <typename T>
T CMB<T>::hb_search(u_int64_t cmb_id, u_int64_t _k){
    
    int idx = hb_kvp_search(cmb_id, _k);
    T ret = hb_read_value(cmb_id, idx);
    return ret;
}

template <typename T>
void CMB<T>::hb_delete(u_int64_t cmb_id, u_int64_t _k){
    
    int idx = hb_kvp_search(cmb_id, _k);
    int num_key = hb_read_num_key(cmb_id);
    for(int i = idx; i < num_key-1; i++){
        u_int64_t next_key = hb_read_key(cmb_id, i+1);
        T next_value = hb_read_value(cmb_id, i+1);
        
        hb_write_key(cmb_id, i, next_key);
        hb_write_value(cmb_id, i, next_value);
        usleep(1);
    }

    hb_write_num_key(cmb_id, num_key-1);
}

template <typename T>
void CMB<T>::hb_write_key(u_int64_t cmb_id, u_int64_t idx, u_int64_t _k){
    
    off_t addr = cmb_id * PAGE_SIZE + idx * (sizeof(u_int64_t)*2 + sizeof(T));
     
    write(addr, &_k, sizeof(u_int64_t)); 
}

template <typename T>
u_int64_t CMB<T>::hb_read_key(u_int64_t cmb_id, u_int64_t idx){
    
    off_t addr = cmb_id * PAGE_SIZE + idx * (sizeof(u_int64_t)*2 + sizeof(T));
     
    u_int64_t ret;
    read(&ret, addr, sizeof(u_int64_t)); 
    return ret;
}

template <typename T>
void CMB<T>::hb_write_value(u_int64_t cmb_id, u_int64_t idx, T _v){
    
    off_t addr = cmb_id * PAGE_SIZE + idx * (sizeof(u_int64_t)*2 + sizeof(T)) + sizeof(u_int64_t);
     
    write(addr, &_v, sizeof(T)); 
}

template <typename T>
T CMB<T>::hb_read_value(u_int64_t cmb_id, u_int64_t idx){
    
    off_t addr = cmb_id * PAGE_SIZE + idx * (sizeof(u_int64_t)*2 + sizeof(T)) + sizeof(u_int64_t);
     
    T ret;
    read(&ret, addr, sizeof(T)); 
    return ret;
}

template <typename T>
void CMB<T>::hb_write_num_key(u_int64_t cmb_id, u_int64_t num_key){
    
    off_t addr = cmb_id * PAGE_SIZE + 128 * (sizeof(u_int64_t)*2 + sizeof(T));
     
    write(addr, &num_key, sizeof(u_int64_t)); 
}

template <typename T>
u_int64_t CMB<T>::hb_read_num_key(u_int64_t cmb_id){
    
    off_t addr = cmb_id * PAGE_SIZE + 128 * (sizeof(u_int64_t)*2 + sizeof(T));
     
    u_int64_t ret;
    read(&ret, addr, sizeof(u_int64_t)); 
    return ret;
}

node_LRU::node_LRU(){
    list_pool = (LRU_list_entry*) malloc((MAX_NUM_NODE+1) * sizeof(LRU_list_entry));
    head = 0;
    tail = 0;
}

node_LRU::~node_LRU(){
    free(list_pool);
}

void node_LRU::dequeue(u_int64_t node_id){

    if(look_up(node_id)){
        if(head == node_id && tail == node_id){
            head = 0;
            tail = 0;
        }
        else if(head == node_id){
            head = list_pool[node_id].next;
            list_pool[head].last = 0;
        }
        else if(tail == node_id){
            tail = list_pool[node_id].last;
            list_pool[tail].next = 0;
        }
        else{
            u_int64_t last_id = list_pool[node_id].last;
            u_int64_t next_id = list_pool[node_id].next;
            list_pool[last_id].next = next_id;
            list_pool[next_id].last = last_id;
        }
    }

    list_pool[node_id].last = 0;
    list_pool[node_id].next = 0;
}

bool node_LRU::look_up(u_int64_t node_id){
    if(head == node_id || tail == node_id)
        return true;

    if(list_pool[node_id].last != 0 || list_pool[node_id].next != 0)
        return true;  

    return false;
}

void node_LRU::insert(u_int64_t node_id){
    dequeue(node_id);

    if(head == 0 && tail == 0){
        head = node_id;
        tail = node_id;
        list_pool[node_id].last = 0;
        list_pool[node_id].next = 0;
    }
    else{
        list_pool[tail].next = node_id;
        list_pool[node_id].last = tail;
        list_pool[node_id].next = 0;
        tail = node_id;
    }
}

u_int64_t node_LRU::pop(){
    if(head == 0 && tail == 0)
        return 0;

    u_int64_t ret;
    if(LRU){
        ret = head;
        if(head == tail){
            head = 0;
            tail = 0;
        }
        else{
            head = list_pool[head].next;
            list_pool[head].last = 0;
        }

        list_pool[ret].last = 0;
        list_pool[ret].next = 0;
    }
    else{
        ret = tail;
        
        if(head == tail){
            head = 0;
            tail = 0;
        }
        else{
            tail = list_pool[tail].last;
            list_pool[tail].next = 0;
        }

        list_pool[ret].last = 0;
        list_pool[ret].next = 0;
    }

    return ret;
}

HB_LRU::HB_LRU(int NUM){
    max_num = NUM;
    head = max_num;
    tail = max_num;
    used = 0;
    list_pool = (HB_LRU_list_entry*) calloc(max_num, sizeof(HB_LRU_list_entry));
    for(int i = 0; i < max_num; i++){
        list_pool[i].last = max_num;
        list_pool[i].next = max_num;
        list_pool[i].used = 0;
    }
}

HB_LRU::~HB_LRU(){
    free(list_pool);
}

bool HB_LRU::full(){
    if(used >= max_num)
        return true;
    else
        return false;
}

u_int64_t HB_LRU::look_up(u_int64_t node_id){
    u_int64_t cmb_idx = head;

    while(cmb_idx < max_num){
        if(list_pool[cmb_idx].NODE_ID == node_id){
            return cmb_idx;            
        }
        cmb_idx = list_pool[cmb_idx].next;
    }
    return max_num;
}

void HB_LRU::reenqueue(u_int64_t cmb_idx){
    mylog << "HB_LRU::reenqueue(cmb_id = " << cmb_idx << ")" << endl;

    if(tail == cmb_idx)
        return;

    if(head == cmb_idx){
        head = list_pool[cmb_idx].next;
        list_pool[head].last = max_num;        
    }
    else{
        u_int64_t last_entry, next_entry;
        last_entry = list_pool[cmb_idx].last;
        next_entry = list_pool[cmb_idx].next;

        list_pool[last_entry].next = next_entry;
        list_pool[next_entry].last = last_entry;
    }

    list_pool[tail].next = cmb_idx;
    list_pool[cmb_idx].last = tail;
    list_pool[cmb_idx].next = max_num;
    tail = cmb_idx;
}

u_int64_t HB_LRU::pop(){
    if(head == max_num && tail == max_num)
        return max_num;

    u_int64_t ret = head;
    if(head == tail){
        head = max_num;
        tail = max_num;
    }
    else{
        head = list_pool[head].next;
        list_pool[head].last = max_num;
    }

    list_pool[ret].last = max_num;
    list_pool[ret].next = max_num;
    list_pool[ret].used = 0;

    used--;

    mylog << "HB_LRU::pop() : cmb_id = " << ret << endl;

    return ret;
}

u_int64_t HB_LRU::enqueue(u_int64_t node_id){
    u_int64_t idx;
    for(idx = 0; idx < max_num; idx++){
        if(list_pool[idx].used == 0)
            break;
    }
    if(idx >= max_num)
        return max_num;

    if(head == max_num && tail == max_num){
        head = idx;
        tail = idx;
        list_pool[idx].last = max_num;
        list_pool[idx].next = max_num;
    }
    else{
        list_pool[tail].next = idx;
        list_pool[idx].last = tail;
        list_pool[idx].next = max_num;
        tail = idx;
    }
    list_pool[idx].NODE_ID = node_id;
    list_pool[idx].used = 1;

    used++;

    mylog << "HB_LRU::enqueue(node_id = " << node_id << "): cmb_id = " << idx << endl;
    
    return idx;
}

u_int64_t HB_LRU::dequeue(u_int64_t node_id){

    u_int64_t idx = tail;
    while(idx < max_num){
        if(list_pool[idx].NODE_ID == node_id)
            break;
        idx = list_pool[idx].last;
    }
    if(idx >= max_num)
        return max_num;
    
    if(head == idx && tail == idx){
        head = max_num;
        tail = max_num;
    }
    else if(head == idx){
        head = list_pool[idx].next;
        list_pool[head].last = max_num;
    }
    else if(tail == idx){
        tail = list_pool[idx].last;
        list_pool[tail].next = max_num;
    }
    else{
        u_int64_t last_entry, next_entry;
        last_entry = list_pool[idx].last;
        next_entry = list_pool[idx].next;
        list_pool[last_entry].next = next_entry;
        list_pool[next_entry].last = last_entry; 
    }
    list_pool[idx].used = 0;
    list_pool[idx].last = max_num;
    list_pool[idx].next = max_num;

    used--;

    mylog << "HB_LRU::dequeue(node_id = " << node_id << "): cmb_id=" << idx << endl;

    return idx;
}

#endif /* B_TREE_H */
