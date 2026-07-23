#include<bits/stdc++.h>

using namespace std;

char ask(int v){
    
    cout<<v<<endl;
    char g;
    cin>>g;
    return g;       
}
int main(){
    int a;
    cin>>a;
    if(a<=1000)
    cout<<"0 "<<a;
    else cout<<"1000 "<<a-1000<<"\n";
}