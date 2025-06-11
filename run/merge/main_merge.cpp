#include <iostream>
#include <sys/stat.h>
#include "../../lib/merge/b_tree.h"
#include "../../lib/YCSB/ycsb_data.h"
using namespace std;

typedef struct TYPE{char str[104];} TYPE;

int random_num(int base, int max){
    return rand() % max + base;
}

bool fileExists(const char* file) {
    struct stat buf;
    return (stat(file, &buf) == 0);
}

void usage(){
    cout << "Format:" << endl;
    cout << "Usage: ./program [OPTION]... {tree_filename}" << endl << endl;
    cout << "Options" << endl;
    cout << "  -d {degree}\t\t\t\tDegree of the B-Tree, only for creating new B-Tree (Default: 128)" << endl;
    cout << "  -l {log file}\t\t\t\tSave all log to newly created file" << endl;
    cout << "  -i {input file}\t\t\tRead input opreations from file" << endl;
}

int main(int argc, char** argv){

    BTree<TYPE>* t;

    srand(time(0));

    int degree = 128;
    char* log_file = NULL;
    char* input_file = NULL;
    char* tree_file = NULL;
    int start = 0;
    int end = 0;
    MODE mode = COPY_ON_WRITE;
    int append = 0;
    int read_threshold = 40;
    bool LRU = true;
    int HB = 0;

    if(argc < 2){
        usage();
        return 0;
    }
    else{
        for(int i = 1; i < argc; i++){          
            if(strcmp(argv[i], "-degree") == 0){ // Degree
                degree = stoi(argv[i+1]);
                i++;
            }
            else if(strcmp(argv[i], "-log") == 0){ // log-file output
                log_file = argv[i+1];
                i++;
            }
            else if(strcmp(argv[i], "-input") == 0){ // input-file
                input_file = argv[i+1];
                i++;
            }
            else if(strcmp(argv[i], "-range") == 0){ // range
                start = atoi(argv[i+1]);
                end = atoi(argv[i+2]);
                i += 2;
            }
            else if(strcmp(argv[i], "-cmb") == 0){ // Set as CMB mode
                mode = REAL_CMB;
            }
            else if(strcmp(argv[i], "-dram") == 0){ // Set as DRAM mode
                mode = DRAM;
            }
            else if(strcmp(argv[i], "-append") == 0){
                append = atoi(argv[i+1]);
                i++;
            }
            else if(strcmp(argv[i], "-thres") == 0){
                read_threshold = atoi(argv[i+1]);
                i++;
            }
            else if(strcmp(argv[i], "-MRU") == 0){
                LRU = false;
            }
            else if(strcmp(argv[i], "-hb") == 0){
                HB = stoi(argv[i+1]);
                i++;
            }
            else{
                tree_file = argv[i];
                break;
            }
        }

        if(tree_file == NULL){
            usage();
            return 0;
        }
    }

    if(input_file == NULL){
        cout << "   Error: Input File Absent" << endl;
        cout << "   Please check the -input " << endl;
        exit(1);
    }

    if(log_file == NULL)
        mylog.open("/dev/null");
    else
        mylog.open(log_file);

    cout << "Create tree <" << tree_file << ">" << endl;
    mylog << "Create tree <" << tree_file << ">" << endl;
    t = new BTree<TYPE>(tree_file, degree, mode, append, read_threshold, LRU, HB);

    t->stat();

    // Has data file as input
    if(input_file){
        YCSB_file *ycsb_file = new YCSB_file(input_file);

		// Extract input filename
		string input_file_str(input_file);
		regex rgx_opf("(\\w+)\\.txt");
		smatch match;
		regex_search(input_file_str, match, rgx_opf);
		string op_file_name(match[1]);

        ofstream op_file;
        op_file.open(op_file_name + ".dat", ios_base::app);

        if(start == 0 && end == 0){
            start = 0;
            end = ycsb_file->get_recordcount();
        }

        cout << "Operation Start!" << endl;
        mylog << "Operation Start!" << endl;
        for(int i = 0; i < ycsb_file->get_recordcount(); i++){
            // Extract data           
            string line; 
            char op;
            u_int64_t key;
            TYPE val;

            if(i < start){
                cout << '\r' << "OP#" << i+1 << " - Skip";
                ycsb_file->skipline();
                continue;
            }
            if(i >= end)
                break;

            line = ycsb_file->readline();
            ycsb_file->lexi(line, &op, &key, val.str);

            if(i < start)
                continue;
            if(i >= end)
                break;

            flash_diff = chrono::microseconds{0};
            cmb_diff = chrono::microseconds{0};

            kv_size = 8 + 104;
            flash_read_size = 0;
            flash_write_size = 0;
            cmb_read_size = 0;
            cmb_write_size = 0;
            structural_change = 'F';

            if(op == 'i'){
                // Insert data
                cout << '\r' << "OP#" << i+1 << " - Insert : " << key << " >> " << val.str;
                mylog << "\n" << "OP#" << i+1 << " - Insert : " << key << " >> " << val.str << endl;;
                auto start = chrono::high_resolution_clock::now();
                t->insertion(key, val);
                auto end = std::chrono::high_resolution_clock::now();
                chrono::duration<double, micro> diff = end - start;
                op_file << "i\t" << key << "\t" << val.str << "\tlat:\t" << diff.count() << "\t" << flash_diff.count() << "\t" << cmb_diff.count() << "\tsize:\t" <<  kv_size << "\t" << flash_read_size << "\t"  << flash_write_size << "\t" << cmb_read_size << "\t" << cmb_write_size << "\t" << structural_change << endl;
            }
            else if(op == 'r'){
                // Read data
                cout << '\r' << "OP#" << i+1 << " - Read : " << key;
                mylog << "\n" << "OP#" << i+1 << " - Read : " << key;
                auto start = chrono::high_resolution_clock::now();
                t->search(key, &val);
                auto end = std::chrono::high_resolution_clock::now();
                cout << " >> " << val.str;
                mylog << " >> " << val.str << endl;
                chrono::duration<double, micro> diff = end - start;
                op_file << "r\t" << key << "\t" << val.str << "\tlat:\t" << diff.count() << "\t" << flash_diff.count() << "\t" << cmb_diff.count() << "\tsize:\t" <<  kv_size << "\t" << flash_read_size << "\t" << flash_write_size << "\t" << cmb_read_size << "\t" << cmb_write_size << "\t" << structural_change << endl;
            }
            else if(op == 'u'){
                // Update data
                cout << '\r' << "OP#" << i+1 << " - Update : " << key << " >> " << val.str;
                mylog << "\n" << "OP#" << i+1 << " - Update : " << key << " >> " << val.str << endl;
                auto start = chrono::high_resolution_clock::now();
                t->update(key, val);
                auto end = std::chrono::high_resolution_clock::now();
                chrono::duration<double, micro> diff = end - start;
                op_file << "u\t" << key << "\t" << val.str << "\tlat:\t" << diff.count() << "\t" << flash_diff.count() << "\t" << cmb_diff.count() << "\tsize:\t" <<  kv_size << "\t" << flash_read_size << "\t" << flash_write_size << "\t" << cmb_read_size << "\t" << cmb_write_size << "\t" << structural_change << endl;

            }
            else if(op == 'd'){
                // Delete data
                cout << '\r' << "OP#" << i+1 << " - Delete : " << key;
                mylog << "\n" << "OP#" << i+1 << " - Delete : " << key << endl;
                auto start = chrono::high_resolution_clock::now();
                t->deletion(key);
                auto end = std::chrono::high_resolution_clock::now();
                chrono::duration<double, micro> diff = end - start;
                op_file << "d\t" << key << "\t" << "no_value" << "\tlat:\t" << diff.count() << "\t" << flash_diff.count() << "\t" << cmb_diff.count() << "\tsize:\t" <<  kv_size << "\t" << flash_read_size << "\t" << flash_write_size << "\t" << cmb_read_size << "\t" << cmb_write_size << "\t" << structural_change << endl;

            }
            else{
                continue;
            }      
        }
	    cout << endl << "Done!" << endl;;

        delete ycsb_file;
        op_file.close();

    }

    mylog.close();
    mylog.open("/dev/null");

    t->inorder_traversal((char*)"tree.dat");

    // For Debug
    //t->display_tree();
    //t->print_used_block_id();

    mylog.close();

    delete t;

    return 0;
}

