/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOrderStatistics.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/

#include "vtkToolkits.h"

#include "vtkOrderStatistics.h"
#include "vtkStatisticsAlgorithmPrivate.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/vector>
#include <vtksys/stl/map>
#include <vtksys/stl/set>
#include <vtksys/ios/sstream>

typedef vtksys_stl::map<double,double> CDF;

vtkStandardNewMacro(vtkOrderStatistics);

// ----------------------------------------------------------------------
vtkOrderStatistics::vtkOrderStatistics()
{
  this->QuantileDefinition = vtkOrderStatistics::InverseCDFAveragedSteps;
  this->NumberOfIntervals = 4; // By default, calculate 5-points statistics

  this->AssessNames->SetNumberOfValues( 1 );
  this->AssessNames->SetValue( 0, "Quantile" );
}

// ----------------------------------------------------------------------
vtkOrderStatistics::~vtkOrderStatistics()
{
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "NumberOfIntervals: " << this->NumberOfIntervals << endl;
  os << indent << "QuantileDefinition: " << this->QuantileDefinition << endl;
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::SetQuantileDefinition( int qd )
{
  switch ( qd )
    {
    case vtkOrderStatistics::InverseCDF:
      break;
    case vtkOrderStatistics::InverseCDFAveragedSteps:
      break;
    default:
      vtkWarningMacro( "Incorrect type of quantile definition: "
                       <<qd
                       <<". Ignoring it." );
      return;
    }

  this->QuantileDefinition =  static_cast<vtkOrderStatistics::QuantileDefinitionType>( qd );
  this->Modified();

  return;
}

// ----------------------------------------------------------------------
bool vtkOrderStatistics::SetParameter( const char* parameter,
                                       int vtkNotUsed( index ),
                                       vtkVariant value )
{
  if ( ! strcmp( parameter, "NumberOfIntervals" ) )
    {
    this->SetNumberOfIntervals( value.ToInt() );

    return true;
    }

  if ( ! strcmp( parameter, "QuantileDefinition" ) )
    {
    this->SetQuantileDefinition( value.ToInt() );

    return true;
    }

  return false;
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::Learn( vtkTable* inData,
                                vtkTable* vtkNotUsed( inParameters ),
                                vtkMultiBlockDataSet* outMeta )
{
  if ( ! inData )
    {
    return;
    }

  if ( ! outMeta )
    {
    return;
    }

  // Summary table: assigns a unique key to each variable
  vtkTable* summaryTab = vtkTable::New();

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable" );
  summaryTab->AddColumn( stringCol );
  stringCol->Delete();

  // The actual histogram table, indexed by the key of the summary
  vtkTable* histogramTab = vtkTable::New();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Key" );
  histogramTab->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkVariantArray* variantCol = vtkVariantArray::New();
  variantCol->SetName( "Value" );
  histogramTab->AddColumn( variantCol );
  variantCol->Delete();

  idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  histogramTab->AddColumn( idTypeCol );
  idTypeCol->Delete();

  // Row to be used to insert into summary table
  vtkVariantArray* row1 = vtkVariantArray::New();
  row1->SetNumberOfValues( 1 );

  // Row to be used to insert into histogram table
  vtkVariantArray* row3 = vtkVariantArray::New();
  row3->SetNumberOfValues( 3 );

  // Insert first row which will always contain the data set cardinality, with key -1
  // NB: The cardinality is calculated in derive mode ONLY, and is set to an invalid value of -1 in
  // learn mode to make it clear that it is not a correct value. This is an issue of database
  // normalization: including the cardinality to the other counts can lead to inconsistency, in particular
  // when the input meta table is calculated by something else than the learn mode (e.g., is specified
  // by the user).
  vtkStdString zString = vtkStdString( "" );
  row3->SetValue( 0, -1 );
  row3->SetValue( 1, zString );
  row3->SetValue( 2, -1 );
  histogramTab->InsertNextRow( row3 );

  // The quantiles table
  vtkTable* quantTab = vtkTable::New();

  stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable" );
  quantTab->AddColumn( stringCol );
  stringCol->Delete();

  idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  quantTab->AddColumn( idTypeCol );
  idTypeCol->Delete();

  double dq = 1. / static_cast<double>( this->NumberOfIntervals );
  for ( int i = 0; i <= this->NumberOfIntervals; ++ i )
    {
    variantCol = vtkVariantArray::New();
    div_t q = div( i << 2, this->NumberOfIntervals );

    if ( q.rem )
      {
      variantCol->SetName( vtkStdString( vtkVariant( i * dq ).ToString() + "-quantile" ).c_str() );
      }
    else
      {
      switch ( q.quot )
        {
        case 0:
          variantCol->SetName( "Minimum" );
          break;
        case 1:
          variantCol->SetName( "First Quartile" );
          break;
        case 2:
          variantCol->SetName( "Median" );
          break;
        case 3:
          variantCol->SetName( "Third Quartile" );
          break;
        case 4:
          variantCol->SetName( "Maximum" );
          break;
        default:
          variantCol->SetName( vtkStdString( vtkVariant( i * dq ).ToString() + "-quantile" ).c_str() );
          break;
        }
      }
    quantTab->AddColumn( variantCol );
    variantCol->Delete();
    }

  // Loop over requests
  vtkIdType nRow = inData->GetNumberOfRows();
  for ( vtksys_stl::set<vtksys_stl::set<vtkStdString> >::iterator rit = this->Internals->Requests.begin();
        rit != this->Internals->Requests.end(); ++ rit )
    {
    // Each request contains only one column of interest (if there are others, they are ignored)
    vtksys_stl::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString col = *it;
    if ( ! inData->GetColumnByName( col ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << col.c_str()
                       << ". Ignoring it." );
      continue;
      }

    // Create entry in summary for variable col and set its index to be the key
    // for values of col in the histogram table
    row1->SetValue( 0, col );

    row3->SetValue( 0, summaryTab->GetNumberOfRows() );

    summaryTab->InsertNextRow( row1 );

    // A quantile row contains: variable name, cardinality, and NumberOfIntervals + 1 quantiles
    vtkVariantArray* rowQuant = vtkVariantArray::New();
    rowQuant->SetNumberOfValues( this->NumberOfIntervals + 3 );

    // Set known row values
    int i = 0;
    rowQuant->SetValue( i ++, col );
    rowQuant->SetValue( i ++, nRow );

    // Calculate and store quantile thresholds
    vtksys_stl::vector<double> quantileThresholds;
    double dh = nRow / static_cast<double>( this->NumberOfIntervals );
    for ( int j = 0; j < this->NumberOfIntervals; ++ j )
      {
      quantileThresholds.push_back( j * dh );
      }

    // Try to downcast column to either data or string arrays for efficient data access
    vtkAbstractArray* arr = inData->GetColumnByName( col );

    // Handle case where input is a data array
    if ( arr->IsA("vtkDataArray") )
      {
      vtkDataArray* darr = vtkDataArray::SafeDownCast( arr );

      // Calculate histogram
      vtksys_stl::map<double,vtkIdType> distr;
      for ( vtkIdType r = 0; r < nRow; ++ r )
        {
        ++ distr[darr->GetTuple1( r )];
        }

      // Store histogram and calculate quantiles at the same time
      vtkIdType sum = 0;
      vtksys_stl::vector<double>::iterator qit = quantileThresholds.begin();
      for ( vtksys_stl::map<double,vtkIdType>::iterator mit = distr.begin();
            mit != distr.end(); ++ mit  )
        {
        // First store histogram row
        row3->SetValue( 1, mit->first );
        row3->SetValue( 2, mit->second );

        histogramTab->InsertNextRow( row3 );

        // Then calculate quantiles
        for ( sum += mit->second; qit != quantileThresholds.end() && sum >= *qit; ++ qit )
          {
          // Mid-point interpolation is available for numeric types only
          if ( sum == *qit
               && this->QuantileDefinition == vtkOrderStatistics::InverseCDFAveragedSteps )
            {
            vtksys_stl::map<double,vtkIdType>::iterator nit = mit;
            rowQuant->SetValue( i ++, ( (++ nit)->first + mit->first ) * .5 );
            }
          else
            {
            rowQuant->SetValue( i ++, mit->first );
            }
          }
        }

      rowQuant->SetValue( i, distr.rbegin()->first );
      quantTab->InsertNextRow( rowQuant );

      }
    // Handle case where input is a string array
    else if ( arr->IsA("vtkStringArray") )
      {
      vtkStringArray* sarr = vtkStringArray::SafeDownCast( arr );

      // Calculate histogram
      vtksys_stl::map<vtkStdString,vtkIdType> distr;
      for ( vtkIdType r = 0; r < nRow; ++ r )
        {
        ++ distr[sarr->GetValue( r )];
        }

      // Store histogram and calculate quantiles at the same time
      vtkIdType sum = 0;
      vtksys_stl::vector<double>::iterator qit = quantileThresholds.begin();
      for ( vtksys_stl::map<vtkStdString,vtkIdType>::iterator mit = distr.begin();
            mit != distr.end(); ++ mit  )
        {
        // First store histogram row
        row3->SetValue( 1, mit->first );
        row3->SetValue( 2, mit->second );

        histogramTab->InsertNextRow( row3 );

        // Then calculate quantiles
        for ( sum += mit->second; qit != quantileThresholds.end() && sum >= *qit; ++ qit )
          {
          rowQuant->SetValue( i ++, mit->first );
          }
        }

      rowQuant->SetValue( i, distr.rbegin()->first );
      quantTab->InsertNextRow( rowQuant );
      }
    else // column is of type vtkVariantArray, which is not supported by this filter
      {
      vtkWarningMacro("Type vtkVariantArray of column "
                      << col.c_str()
                      << " not supported. Ignoring it." );
      }

    // Clean up
    rowQuant->Delete();
    }

  // Finally set first and second blocks of output meta port
  outMeta->SetNumberOfBlocks( 3 );
  outMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Summary" );
  outMeta->SetBlock( 0, summaryTab );
  outMeta->GetMetaData( static_cast<unsigned>( 1 ) )->Set( vtkCompositeDataSet::NAME(), "Histogram" );
  outMeta->SetBlock( 1, histogramTab );
  outMeta->GetMetaData( static_cast<unsigned>( 2 ) )->Set( vtkCompositeDataSet::NAME(), "Quantiles" );
  outMeta->SetBlock( 2, quantTab );

  // Clean up
  summaryTab->Delete();
  histogramTab->Delete();
  row1->Delete();
  row3->Delete();
  quantTab->Delete();

  return;
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::Derive( vtkMultiBlockDataSet* inMeta )
{
  if ( ! inMeta || inMeta->GetNumberOfBlocks() < 2 )
    {
    return;
    }

  vtkTable* summaryTab = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
  if ( ! summaryTab  )
    {
    return;
    }

  vtkTable* histogramTab = vtkTable::SafeDownCast( inMeta->GetBlock( 1 ) );
  if ( ! histogramTab  )
    {
    return;
    }

  // Create table for derived statistics
  vtkIdType nRow = histogramTab->GetNumberOfRows();
  vtkTable* derivedTab = vtkTable::New();

  // Iterate over rows of primary table
  for ( int i = 0; i < nRow; ++ i )
    {
    // Do nothing for now
    }

  // Clean up
  derivedTab->Delete();
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::Test( vtkTable* inData,
                               vtkMultiBlockDataSet* inMeta,
                               vtkTable* outMeta )
{
  if ( ! inMeta )
    {
    return;
    }

  vtkTable* quantTab = vtkTable::SafeDownCast( inMeta->GetBlock( 2 ) );
  if ( ! quantTab )
    {
    return;
    }

  if ( ! outMeta )
    {
    return;
    }

  // Prepare columns for the test:
  // 0: variable name
  // 1: Maximum vertical distance between CDFs
  // 2: Kolmogorov-Smirnov test statistic (the above times the square root of the cardinality)
  // NB: These are not added to the output table yet, for they will be filled individually first
  //     in order that R be invoked only once.
  vtkStringArray* nameCol = vtkStringArray::New();
  nameCol->SetName( "Variable" );

  vtkDoubleArray* distCol = vtkDoubleArray::New();
  distCol->SetName( "Maximum Distance" );

  vtkDoubleArray* statCol = vtkDoubleArray::New();
  statCol->SetName( "Kolomogorov-Smirnov" );

  // Downcast columns to string arrays for efficient data access
  vtkStringArray* vars = vtkStringArray::SafeDownCast( quantTab->GetColumnByName( "Variable" ) );

  // Prepare storage for quantiles and model CDFs
  vtkIdType nQuant = quantTab->GetNumberOfColumns() - 2;
  double* quantiles = new double[nQuant];

  // Loop over requests
  vtkIdType nRowQuant = quantTab->GetNumberOfRows();
  vtkIdType nRowData = inData->GetNumberOfRows();
  double inv_nq =  1. / nQuant;
  double inv_card = 1. / nRowData;
  double sqrt_card = sqrt( static_cast<double>( nRowData ) );
  for ( vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
        rit != this->Internals->Requests.end(); ++ rit )
    {
    // Each request contains only one column of interest (if there are others, they are ignored)
    vtksys_stl::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString varName = *it;
    if ( ! inData->GetColumnByName( varName ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varName.c_str()
                       << ". Ignoring it." );
      continue;
      }

    // Find the model row that corresponds to the variable of the request
    vtkIdType r = 0;
    while ( r < nRowQuant && vars->GetValue( r ) != varName )
      {
      ++ r;
      }
    if ( r >= nRowQuant )
      {
      vtkWarningMacro( "Incomplete input: model does not have a row "
                       << varName.c_str()
                       <<". Cannot test." );
      continue;
      }

    // First iterate over all observations to calculate empirical PDF
    CDF cdfEmpirical;
    for ( vtkIdType j = 0; j < nRowData; ++ j )
      {
      // Read observation and update PDF
      double x = inData->GetValueByName( j, varName ).ToDouble();

      cdfEmpirical[x] += inv_card;
      }

    // Now integrate to obtain empirical CDF
    double sum = 0.;
    for ( CDF::iterator cit = cdfEmpirical.begin(); cit != cdfEmpirical.end(); ++ cit )
      {
      sum += cit->second;
      cit->second = sum;
      }

    // Sanity check: verify that empirical CDF = 1
    if ( fabs( sum - 1. ) > 1.e-6 )
      {
      vtkWarningMacro( "Incorrect empirical CDF for variable:"
                       << varName.c_str()
                       << ". Ignoring it." );

      continue;
      }

    // Retrieve quantiles to calculate model CDF and insert value into empirical CDF
    for ( vtkIdType i = 0; i < nQuant; ++ i )
      {
      // Read quantile and update CDF
      quantiles[i] = quantTab->GetValue( r, i + 2 ).ToDouble();

      // Update empirical CDF if new value found (with unknown ECDF)
      vtksys_stl::pair<CDF::iterator,bool> result
        = cdfEmpirical.insert( vtksys_stl::pair<double,double>( quantiles[i], -1 ) );
      if ( result.second == true )
        {
        CDF::iterator eit = result.first;
        // Check if new value has no predecessor, in which case CDF = 0
        if ( eit ==  cdfEmpirical.begin() )
          {
          result.first->second = 0.;
          }
        else
          {
          -- eit;
          result.first->second = eit->second;
          }
        }
      }

    // Iterate over all CDF jump values
    int currentQ = 0;
    double mcdf = 0.;
    double Dmn = 0.;
    for ( CDF::iterator it = cdfEmpirical.begin(); it != cdfEmpirical.end(); ++ it )
      {
      // If observation is smaller than minimum then there is nothing to do
      if ( it->first >= quantiles[0] )
        {
        while ( it->first >= quantiles[currentQ] && currentQ < nQuant )
          {
          ++ currentQ;
          }

        // Calculate model CDF at observation
        mcdf = currentQ * inv_nq;
        }

      // Calculate vertical distance between CDFs and update maximum if needed
      double d = fabs( it->second - mcdf );
      if ( d > Dmn )
        {
        Dmn =  d;
        }
      }

    // Insert variable name and calculated Kolmogorov-Smirnov statistic
    // NB: R will be invoked only once at the end for efficiency
    nameCol->InsertNextValue( varName );
    distCol->InsertNextTuple1( Dmn );
    statCol->InsertNextTuple1( sqrt_card * Dmn );
    } // rit

  // Now, add the already prepared columns to the output table
  outMeta->AddColumn( nameCol );
  outMeta->AddColumn( distCol );
  outMeta->AddColumn( statCol );

  // Clean up
  delete [] quantiles;
  nameCol->Delete();
  distCol->Delete();
  statCol->Delete();
}

// ----------------------------------------------------------------------
class TableColumnBucketingFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkAbstractArray* Data;
  vtkVariantArray* Quantiles;

  TableColumnBucketingFunctor( vtkAbstractArray* vals,
                               vtkVariantArray* quantiles )
  {
    this->Data = vals;
    this->Quantiles = quantiles;
  }
  virtual ~TableColumnBucketingFunctor() { }
  virtual void operator() ( vtkVariantArray* result,
                            vtkIdType id )
  {
    vtkVariant x = this->Data->GetVariantValue( id );

    if ( x < this->Quantiles->GetValue( 2 ) ) // Value #0 is the variable name and #1 is the cardinality
      {
      // x is smaller than lower bound
      result->SetNumberOfValues( 1 );
      result->SetValue( 0, 0 );

      return;
      }

    vtkIdType n = this->Quantiles->GetNumberOfValues() + 2;
    vtkIdType q = 3;
    while ( q < n && x > this->Quantiles->GetValue( q ) )
      {
      ++ q;
      }

    result->SetNumberOfValues( 1 );
    result->SetValue( 0, q - 2 );
  }
};

// ----------------------------------------------------------------------
void vtkOrderStatistics::SelectAssessFunctor( vtkTable* outData,
                                              vtkDataObject* inMetaDO,
                                              vtkStringArray* rowNames,
                                              AssessFunctor*& dfunc )
{
  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta
       || inMeta->GetNumberOfBlocks() < 1 )
    {
    return;
    }

  vtkTable* quantTab = vtkTable::SafeDownCast( inMeta->GetBlock( 2 ) );
  if ( ! quantTab )
    {
    return;
    }

  vtkStdString varName = rowNames->GetValue( 0 );

  // Downcast meta columns to string arrays for efficient data access
  vtkStringArray* vars = vtkStringArray::SafeDownCast( quantTab->GetColumnByName( "Variable" ) );
  if ( ! vars )
    {
    dfunc = 0;
    return;
    }

  // Loop over parameters table until the requested variable is found
  vtkIdType nRowP = quantTab->GetNumberOfRows();
  for ( int r = 0; r < nRowP; ++ r )
    {
    if ( vars->GetValue( r ) == varName )
      {
      // Grab the data for the requested variable
      vtkAbstractArray* vals = outData->GetColumnByName( varName );
      if ( ! vals )
        {
        dfunc = 0;
        return;
        }

      dfunc = new TableColumnBucketingFunctor( vals, quantTab->GetRow( r ) );
      return;
      }
    }

  // The variable of interest was not found in the parameter table
  dfunc = 0;
}
