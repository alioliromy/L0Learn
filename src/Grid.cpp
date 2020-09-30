#include "Grid.h"

// Assumes PG.P.Specs have been already set
template <class T>
Grid<T>::Grid(const T& X, const arma::vec& y, const GridParams<T>& PGi) {
    PG = PGi;
    
    std::tie(Xscaled, BetaMultiplier, meanX, meany, scaley) = Normalize(X, 
             y, yscaled, !PG.P.Specs.Classification, PG.intercept);
    
    // Must rescale bounds by BetaMultiplier inorder for final result to conform to bounds
    PG.P.Lows /= BetaMultiplier;
    PG.P.Highs /= BetaMultiplier;
}

template <class T>
void Grid<T>::Fit() {
    
    std::vector<std::vector<std::unique_ptr<FitResult<T>>>> G;
    if (PG.P.Specs.L0) {
        G.push_back(std::move(Grid1D<T>(Xscaled, yscaled, PG).Fit()));
        Lambda12.push_back(0);
    } else {
        G = std::move(Grid2D<T>(Xscaled, yscaled, PG).Fit());
    }
    
    Lambda0 = std::vector< std::vector<double> >(G.size());
    NnzCount = std::vector< std::vector<std::size_t> >(G.size());
    Solutions = std::vector< std::vector<arma::sp_mat> >(G.size());
    Intercepts = std::vector< std::vector<double> >(G.size());
    Converged = std::vector< std::vector<bool> >(G.size());
    
    //for (auto &g : G)
    // Rcpp::Rcout << "Starting G iteration \n";
    for (std::size_t i=0; i<G.size(); ++i) {
        // Rcpp::Rcout << "Lambda12 " << G[i][0]->ModelParams[1] << ", " << G[i][0]->ModelParams[2] <<  "\n";
        if (PG.P.Specs.L0L1){ 
            // Rcpp::Rcout << "Lambda12.push_back(G[i][0]->ModelParams[1]);: "<< G[i][0]->ModelParams[1] << " \n";
            Lambda12.push_back(G[i][0]->ModelParams[1]); 
        } else if (PG.P.Specs.L0L2) { 
            Lambda12.push_back(G[i][0]->ModelParams[2]); 
        }
        
        // Rcpp::Rcout << "Size of G[" << i << "] = " << G[i].size() << " \n";
        for (auto &g : G[i]) {
            Lambda0[i].push_back(g->ModelParams[0]);
            
            NnzCount[i].push_back(g->B.n_nonzero);
            
            if (g->IterNum != PG.P.MaxIters){
                Converged[i].push_back(true);
            } else {
                Converged[i].push_back(false);
            }
            
            arma::sp_mat B_unscaled;
            double intercept;
            
            
            std::tie(B_unscaled, intercept) = DeNormalize(g->B, BetaMultiplier, meanX, meany);
            Solutions[i].push_back(B_unscaled);
            /* scaley is 1 for classification problems.
             *  g->intercept is 0 unless specifically optimized for in:
             *       classification
             *       sparse regression and intercept = true
             */
            Intercepts[i].push_back(scaley*g->intercept + intercept);
        }
    }
}

template class Grid<arma::mat>;
template class Grid<arma::sp_mat>;
