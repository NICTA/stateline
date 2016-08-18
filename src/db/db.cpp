//!
//! Contains implementation of the database structure used for persistent
//! storage of the MCMC data.
//!
//! \file db/db.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "db/db.hpp"

#include <iostream>

namespace stateline { namespace db {

DBWriter::DBWriter(const DBSettings& settings)
  : settings_{settings}
  , file_{settings.filename.c_str(), H5F_ACC_EXCL}
{
  for (std::size_t i = 0; i < settings.numChains; i++)
  {
    const std::string groupName = "/chain" + std::to_string(i);

    // Create a group for each chain
    groups_.push_back(file_.createGroup(groupName.c_str()));

    // Create the dataspace for the samples
    const hsize_t dims[] = { 0, settings.numDims };
    const hsize_t maxdims[] = { H5S_UNLIMITED, settings.numDims };
    H5::DataSpace dataspace{2, dims, maxdims};

    // Enable chunking
    H5::DSetCreatPropList prop;
    const hsize_t chunk_dims[] = { settings.chunkSize, settings.numDims };
    prop.setChunk(2, chunk_dims);

    // Create a dataset to hold its samples
    datasets_.push_back(file_.createDataSet(
      (groupName + "/samples").c_str(),
      H5::PredType::NATIVE_DOUBLE,
      dataspace,
      prop
    ));
  }

  // Reserve the samples buffer
  sbuffer_.reserve(settings.chunkSize * settings.numDims);
}

void DBWriter::appendStates(std::size_t id, std::vector<mcmc::State>::iterator start,
    std::vector<mcmc::State>::iterator end)
{
  // TODO: needs to be transactional
  assert(id >= 0 && id < datasets_.size());

  // Copy in the samples
  sbuffer_.clear();
  for (auto it = start; it != end; ++it)
  {
    for (std::size_t i = 0; i < it->sample.size(); i++)
      sbuffer_.push_back(it->sample(i));
  }

  // Define a memory dataspace to copy all the data to the file dataspace
  const hsize_t dims[] = { sbuffer_.size() };
  H5::DataSpace memspace(1, dims);


  // Extend the dataspace
  H5::DataSpace dataspace = datasets_[id].getSpace();

  const hsize_t oldLength = dataspace.getSimpleExtentNpoints() / settings_.numDims;
  const hsize_t offset[] = { oldLength, 0 };
  const hsize_t stride[] = { 1, 1 };
  const hsize_t count[] = { sbuffer_.size() / settings_.numDims, settings_.numDims };
  const hsize_t block[] = { 1, 1 };
  const hsize_t newSize[] = { offset[0] + count[0], settings_.numDims };

  datasets_[id].extend(newSize);

  // Select the new (extended) hyperslab
  H5::DataSpace filespace = datasets_[id].getSpace();

  filespace.selectHyperslab(H5S_SELECT_SET, count, offset, stride, block);

  datasets_[id].write(sbuffer_.data(), H5::PredType::NATIVE_DOUBLE, memspace, filespace);
}

} }
