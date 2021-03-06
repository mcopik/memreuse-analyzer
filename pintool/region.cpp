
#include <iomanip>
#include <cstdio>

#include "region.hpp"

void Region::print(std::ofstream & of)
{
  of << "#region " << _region_name << " " << _counts.size() << '\n';
  for(iter_t it = _counts.begin(); it != _counts.end(); ++it) {
    of << std::hex << (*it).first << std::dec << " ";
    of << (*it).second.read_count << " " << (*it).second.write_count << '\n';
  }
}

int32_t Region::count() const
{
  return _count;
}

uintptr_t Region::align_address(uintptr_t addr)
{
  if(addr % _cacheline_size)
    return addr - addr % _cacheline_size;
  else
    return addr;
}

void Region::read(uintptr_t addr, int32_t size)
{

  for(int i = 0; i < size; i += MEMORY_ACCESS_GRANULARITY) {

    auto aligned_addr = align_address(addr + i);
    iter_t it = _counts.find(aligned_addr);

    if(it == _counts.end()) {

      _counts.insert(std::make_pair(aligned_addr, AccessStats{1, 0}));

    } else {
      (*it).second.read_count += 1;
    }
  }

}

void Region::write(uintptr_t addr, int32_t size)
{

  for(int i = 0; i < size; i += MEMORY_ACCESS_GRANULARITY) {

    auto aligned_addr = align_address(addr + i);
    iter_t it = _counts.find(aligned_addr);

    if(it == _counts.end()) {
      _counts.insert(std::make_pair(aligned_addr, AccessStats{0, 1}));
    } else {
      (*it).second.write_count += 1;
    }
  }

}

void Region::reset()
{
  _counts.clear();
  _count++;
}

Regions::~Regions()
{
  this->close();
}

void Regions::filename(const std::string& file_name)
{
  _file_name = file_name;
}

void Regions::close()
{
  // Ignore data from unfinished regions
  log_file.close();
  for(iter_t it = _regions.begin(); it != _regions.end(); ++it)
    delete (*it).second;
}

Region* Regions::start_region(std::string name, int cacheline_size)
{
  iter_t it = _regions.find(name);

  if(it  == _regions.end()) {

    Region* region = new Region{name, cacheline_size};
    _regions.insert(std::make_pair(name, region));
    return region;

  } else {
    return (*it).second;
  }

}

void Regions::end_region(Region* region)
{
  char counter[17];
  sprintf(counter, "%d", region->_count);
  std::string file = _file_name+ "." + region->_region_name + "." + counter;
  log_file.open(file.c_str(), std::ios::out);
  region->print(log_file);
  region->reset();
  log_file.close();
}

bool HostEvent::empty() const
{
  return region_name.empty() || counter == -1;
}

void HostEvent::print(std::ofstream & of, const std::string & prefix) const
{
  of << prefix << "{\n";
  of << prefix + prefix << "\"region\":\t\"" << region_name << "\",\n";
  of << prefix + prefix << "\"counter\":\t" << counter << "\n";
  of << prefix << "}";
}

void HostRegionChange::print(std::ofstream & of, const std::string & prefix) const
{
  of << prefix << "{\n";

  if(!before.empty()) {

    of << prefix << prefix << "\"before\":";
    before.print(of, prefix + prefix);

    // JSON does not permit trailing comma
    if(!after.empty())
      of << prefix << ",\n";
    else
      of << prefix << '\n';

  }

  if(!after.empty()) {

    of << prefix << prefix << "\"after\": ";
    after.print(of, prefix + prefix + prefix);

  }

  of << prefix << "}";

}

