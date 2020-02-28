/*!
 * @文件描述: RB tree 的实现
 *
 * @创建者：张纪杨
 *
 * @创建日期: 2019/02/20
 *
 * @版权声明：PKUSZ
 *
 * @缩进：shiftwidth=4 tabstop=4
 */
#include <vector>
#include <assert.h>
#include <iostream>

namespace TEST {

void mySort(std::vector<int>& data, int l, int h) {
    if(l >= h) return;
    int i = l - 1, pivot = data[h];
    for(int j = l; j < h; ++j) {
        if(data[j] <= pivot) {
            ++i;
            std::swap(data[i], data[j]);
        }
    }
    std::swap(data[++i], data[h]);
    mySort(data, l, i-1);
    mySort(data, i+1, h);
}

enum Color {red, black};

struct Node {
    Color color; 
    int key;
    Node *left, *right, *p;
    Node(int val, Color _color = red) : color(_color), key(val) {}
};

class RB_Tree {
public:
    int node_count;
    Node* nil;
    Node* rbRoot;
    RB_Tree() : node_count(0), nil(new Node(0, black)), rbRoot(nil) {}

    void left_rotate(Node* z) {
        assert(z != nullptr && z->right != nil);
        Node* right = z->right;
        z->right = right->left;
        if(right->left != nil) right->left->p = z;
        right->left = z;
        right->p = z->p;
        // 如果right旋转后是根节点
        if(z->p == nil) rbRoot = right;
        else if(z->p->left == z) z->p->left = right;
        else z->p->right = right;
        z->p = right;
    }

    void right_rotate(Node *z) {
        assert(z != nil && z != nullptr && z->left != nil);
        Node *left = z->left;
        z->left = left->right;
        if(left->right != nil) left->right->p = z;
        left->right = z;
        left->p = z->p;
        if(z->p == nil) rbRoot = left;
        else if(z->p->left == z) z->p->left = left;
        else z->p->right = left;
        z->p = left;
    }

    void rb_insert_fixup(Node* z) {
        assert(z->color == red);
        // 叔节点
        Node *y;
        while(z->p->color == red) {
            if(z->p == z->p->p->left) {
                y = z->p->p->right;
                if(y->color == red) {
                    // 父和叔两个原本都是红的，都变成黑，不影响5条性质
                    // 因为黑高没有改变（注意祖父一定是黑的，因为父是红的）
                    y->color = black;
                    z->p->color = black;
                    z->p->p->color = red;
                    z = z->p->p;
                } else if(z == z->p->right) {
                    z = z->p; 
                    left_rotate(z);
                } else {
                    z->p->color = black; 
                    z->p->p->color = red;
                    right_rotate(z->p->p);
                }
            } else {
                y = z->p->p->left;
                if(y->color == red) {
                    y->color = black;
                    z->p->color = black;
                    z->p->p->color = red;
                    z = z->p->p;
                } else if(z->p->left == z) {
                    z = z->p;
                    right_rotate(z);
                } else {
                    z->p->color = black;
                    z->p->p->color = red;
                    left_rotate(z->p->p);
                }
            }
        }
        rbRoot->color = black;
    }

    void insert(int key) {
        Node *z = new Node(key);

        Node *y = nil;
        Node *x = rbRoot;
        while(x != nil) {
            y = x;
            if(x->key > z->key) 
                x = x->left;
            else 
                x = x->right;
        }

        if(y == nil) rbRoot = z;
        else if(y->key < z->key) y->right = z;
        else y->left = z;

        z->left = nil; z->right = nil;
        z->p = y;
        rb_insert_fixup( z);
        node_count++;
    }

    Node* minimum(Node *z) {
        while(z != nil && z->left != nil) {
            z = z->left;
        }
        return z;
    }

    // 用v替代u
    void transplant(Node *u, Node *v) {
        assert(u != nil);
        if(u->p == nil) 
            rbRoot = v;
        else if(u->p->left == u)
            u->p->left = v;
        else
            u->p->right = v;
        v->p = u->p;
    }

   /* TODO: 写不写呢？  <16-07-18, pku219> */
    /*
    void rb_delete(Node *z) {
        Node *y = z;
        Color y_original_color = z->color;
        if(z->left == nil) {
            x = z->right;
            transplant(z, z->right);
        } else if (z->right == nil) {
            x = z->left;
            transplant(z, z->left);
        } else {
            // 找到后继
            y = minimum(z->right);
            if(y == z->right) {
                x->p = 
            }
        }
    }
    */

    std::vector<int> traverse() {
        std::vector<int> ans;
        traverse_r(rbRoot, ans);
        return ans;
    }

    void traverse_r(Node *cur, std::vector<int>& ans) {
        assert(cur != nullptr);
        if(cur == nil) return;
        // std::cout << "tesdgds" << std::endl;
        traverse_r(cur->left, ans);
        ans.push_back(cur->key);
        // std::cout << cur->key << std::endl;
        traverse_r(cur->right, ans);
    }
};

}
