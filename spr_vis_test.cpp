#include <cassert>
#include <vector>
#include <algorithm>
#include <iterator>

#include <boost/dynamic_bitset.hpp> 
#include <memory>
#include <map>
#include <boost/tr1/unordered_map.hpp>
#include "ivymike/tree_parser.h"
#include "ivymike/tree_split_utils.h"

using ivy_mike::tree_parser_ms::lnode;
using ivy_mike::tree_parser_ms::parser;
using ivy_mike::tree_parser_ms::ln_pool;


boost::dynamic_bitset<> tip_list_to_split( const std::vector<std::string> &split, const std::vector<std::string> &sorted_names ) {
 
    boost::dynamic_bitset<> bitset(sorted_names.size());
    
    for( auto it = split.begin(); it != split.end(); ++it ) {
        auto sit = std::lower_bound(sorted_names.begin(), sorted_names.end(), *it );
        
        assert( sit != sorted_names.end() );
        
        
//         std::cout << *sit << " " << *it << "\n";
        assert( *sit == *it );
        
        size_t idx = std::distance( sorted_names.begin(), sit );
        assert( idx < sorted_names.size() );
        
        bitset[idx] = true;
      
    }
    return bitset;
}

class trace_element {
public:
    enum trace_type {
        tree,
        subtree,
        insertion,
        none
    };
    
    virtual trace_type get_type() = 0;
};


class trace_tree : public trace_element {
public:
    
    trace_tree( lnode *t ) : tree_(t) {}
    
    virtual trace_type get_type() {
        return trace_type::tree;
    }
    
    lnode *get_tree() {
        return tree_;
    }
    
private:
    lnode *tree_;
};

class trace_subtree : public trace_element {
public:
    template<typename iiter>
    trace_subtree( iiter first, iiter last ) : tip_list_(first, last) {
        std::sort( tip_list_.begin(), tip_list_.end() );
    }
    
    virtual trace_type get_type() {
        return trace_type::subtree;
    }
    
    const std::vector<std::string> &get_tip_list() {
        return tip_list_;
    }
    
private:
    std::vector<std::string> tip_list_;
};

class trace_insertion : public trace_element {
public:
    template<typename iiter>
    trace_insertion( iiter first, iiter last, double score ) : split_(first, last), score_(score) {
        std::sort( split_.begin(), split_.end() );
    }
    
    virtual trace_type get_type() {
        return trace_type::insertion;
    }
    
    const std::vector<std::string> &get_split() {
        return split_;
    }
    
    double get_score() const {
        return score_;
    }
    
private:
    std::vector<std::string> split_;
    const double score_;
};

// template<typename T>
// std::unique_ptr<T> make_uniqueptr( T *p ) {
//     return std::unique_ptr<T>(p);
// }
class trace_reader {
public:
    trace_reader( const char *filename, ln_pool *pool ) : pool_(pool), element_type_(trace_element::none), line_count_(0) {
        is_.open( filename );
        
        assert( is_.good() );
    }
    
//     ~trace_reader() {
//      
//     }
    
    
    void dump_position() {
        std::cerr << "trace reader lines: " << line_count_ << "\n";
        std::cerr << line_ << "\n";   
    }
    
    trace_element::trace_type next() {
        while( true ) {
            if( is_.eof() ) {
                element_type_ = trace_element::none;
                
                return element_type_;
            
            }
            
            
         
            std::getline(is_, line_);
            ++line_count_;
            std::stringstream ss(line_);
            std::string token;

            ss >> token;
            
//             std::cout << "token: " << token << "\n";
            
            if( token == "@tree" || token == "@tree:" ) {
                element_type_ = trace_element::tree;
            } else if( token == "@subtree" ) {
                element_type_ = trace_element::subtree;
            } else if( token == "@insertion" ) {
                element_type_ = trace_element::insertion;
            } else {
                continue; // ignore anything else
            }
            
            return element_type_;
              
            
            
        }
    }
    
    
    trace_tree get_tree() {
        if( element_type_ != trace_element::tree ) {
            throw std::runtime_error( "element_type_ != trace_element::tree" );
        }
        
        std::string::iterator first = std::find( line_.begin(), line_.end(), '(' );
        assert( first != line_.end() );
        
        parser p( first, line_.end(), *pool_ );
                
        lnode *t = p.parse();
            
        return trace_tree(t);
        
    }
    
    trace_subtree get_subtree() {
        if( element_type_ != trace_element::subtree ) {
            throw std::runtime_error( "element_type_ != trace_element::subtree" );
    
        }
    
        std::string::iterator first = std::find( line_.begin(), line_.end(), '(' );
        assert( first != line_.end() );
        ++first;
        
        std::string::iterator last = std::find( line_.begin(), line_.end(), ')' );
        assert( last != line_.end() );
        
        // that's a bit clumsy:
        std::string tmp( first, last );
//         std::cout << "tmp: " << tmp << "\n";
        
        std::stringstream ss(tmp);
        
        std::istream_iterator<std::string> sfirst(ss);
        std::istream_iterator<std::string> slast; 
        return trace_subtree( sfirst, slast );
    }
    
    trace_insertion get_insertion() {
        if( element_type_ != trace_element::insertion ) {
            throw std::runtime_error( "element_type_ != trace_element::insertion" );
    
        }
        
        double score = 1/0.0;
        {
            std::stringstream ss( line_ );
            std::string tmp;
            ss >> tmp;
            assert( tmp == "@insertion" );
            ss >> tmp;
            assert( !tmp.empty() );
            
            score = atof( tmp.c_str() );
            
        }
        // now extract the tip list between the ( )
        // that's all a bit clumsy...
        
        std::string::iterator first = std::find( line_.begin(), line_.end(), '(' );
        assert( first != line_.end() );
        ++first;
        
        std::string::iterator last = std::find( line_.begin(), line_.end(), ')' );
        assert( last != line_.end() );
        
      
        std::string tmp( first, last );
//         std::cout << "tmp: " << tmp << "\n";
        
        std::stringstream ss(tmp);
        
        std::istream_iterator<std::string> sfirst(ss);
        std::istream_iterator<std::string> slast; 
        return trace_insertion( sfirst, slast, score );

    }
    
    
private:
    std::ifstream is_;
    ln_pool * const pool_; // this is a non owning shared ptr! Switch to shared_ptr at some point!
    
    std::string line_;
    trace_element::trace_type element_type_;
    
    size_t line_count_;
};


int main( int argc, char *argv[] ) {
    assert( argc == 2 );
 
    const char *trace_name = argv[1];
    ln_pool pool;
    
    trace_reader tr( trace_name, &pool );
    
    while( true ) { 
            
        trace_element::trace_type t = tr.next();
        
        if( t == trace_element::none ) {
            throw std::runtime_error( "end of trace while looking for first tree\n" );
        } else if( t == trace_element::tree ) {
            break;
        }
    }
    size_t tree_count = 0;
    bool do_exit = false;
    while( !do_exit ) {
        lnode *tree = 0;
        
        std::cout << "tree\n";
                
        trace_tree t = tr.get_tree();
        
        
        ++tree_count;
        
        
        pool.clear();
        pool.mark(t.get_tree());
        pool.sweep();
        

        tree = t.get_tree();
        
        
        
//         {
//             std::ofstream os ( "cur_tree" );
//             ivy_mike::tree_parser_ms::print_newick( tree, os );
//         }
     
        assert( tree != 0 );
        //getchar();
        
        std::vector<lnode *> sorted_tips;
        
        typedef std::tr1::unordered_map<boost::dynamic_bitset<>, lnode*, ivy_mike::bitset_hash > split_to_node_map;
        split_to_node_map split_to_node;
        
        {
            std::vector<lnode* > nodes;
            std::vector<boost::dynamic_bitset<> > splits;
            
            
            // get the lists of splits and correponding edges and put them into split_to_edge map
            ivy_mike::get_all_splits_by_node( tree, nodes, splits, sorted_tips );
            
            std::cout << "size: " << nodes.size() << "\n";
            
            for( size_t i = 0; i < splits.size(); ++i ) {
                split_to_node.insert( std::make_pair( splits.at(i), nodes.at(i) ) ); // TODO: change this to emplace and move semantics
            }
        }
        std::vector<std::string> sorted_names;
        for( auto it = sorted_tips.begin(); it != sorted_tips.end(); ++it ) {
            sorted_names.push_back((*it)->m_data->tipName);
        }
        
        
        
        while( true ) { 
            
            trace_element::trace_type t = tr.next();
            
            if( t == trace_element::none ) {
                do_exit = true;
                break; // end of stream
            } else if( t == trace_element::insertion ) {
                trace_insertion pos = tr.get_insertion();
                
                
                
                boost::dynamic_bitset<> split = tip_list_to_split( pos.get_split(), sorted_names );
                
                //             split.flip();
                
                split_to_node_map::iterator it = split_to_node.find( split );
                
                //             if( it == split_to_edge.end() ) {
                    //                 split.flip();
                //                 it = split_to_edge.find( split );
                //             }
                
                if( it == split_to_node.end() ) {
                    {
                        std::ofstream os ( "error_tree" );
                        ivy_mike::tree_parser_ms::print_newick( tree, os );
                    }
                    tr.dump_position();
                    throw std::runtime_error( "split not found" );
                }
                
                std::cout << "insertion:  " << *(it->second->m_data) << " " << pos.get_score() << "\n";
                
            } else if( t == trace_element::subtree ) {
                trace_subtree st = tr.get_subtree();
                
                std::cout << "subtree: " << st.get_tip_list().size() << "\n";
                
                boost::dynamic_bitset<> split = tip_list_to_split( st.get_tip_list(), sorted_names );
                
                //split.flip();
                
                split_to_node_map::iterator it = split_to_node.find( split );
                
                //             if( it == split_to_edge.end() ) {
                    //                 split.flip();
                //                 it = split_to_edge.find( split );
                //             }
                
                assert( it != split_to_node.end() );
                
                std::cout << "split " << split.count() << " " << it->first.count() << "\n";
                std::cout << "node: " << *(it->second->m_data) << "\n";
                
                
            } else if( t == trace_element::tree ){
                break;
            }
        }
    }
    
    return 0;
}

int main2( int argc, char *argv[] ) {
    
    assert( argc == 2 );
    
    const char *tree_name = argv[1];

    ln_pool pool;
    parser p( tree_name, pool );
    
    lnode *t = p.parse();
    
    std::vector< std::pair< lnode*, lnode* > > edges;
    std::vector<boost::dynamic_bitset<> > splits;
    std::vector<lnode *> sorted_tips;
    
    ivy_mike::get_all_splits( t, edges, splits, sorted_tips );
    
    std::vector<std::string> sorted_names;
    for( auto it = sorted_tips.begin(); it != sorted_tips.end(); ++it ) {
        sorted_names.push_back((*it)->m_data->tipName);
    }
    
    std::vector<std::string> split_names = { "Sbay", "Scas", "Sklu", "Calb" };
    boost::dynamic_bitset<> split = tip_list_to_split( split_names, sorted_names );
    
    return 0;
}