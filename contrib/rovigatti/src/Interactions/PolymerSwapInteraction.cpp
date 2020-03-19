#include "PolymerSwapInteraction.h"

#include "Particles/CustomParticle.h"
#include <fstream>

PolymerSwapInteraction::PolymerSwapInteraction() :
				BaseInteraction<PolymerSwapInteraction>() {
	_int_map[BONDED] = &PolymerSwapInteraction::pair_interaction_bonded;
	_int_map[NONBONDED] = &PolymerSwapInteraction::pair_interaction_nonbonded;
}

PolymerSwapInteraction::~PolymerSwapInteraction() {

}

void PolymerSwapInteraction::get_settings(input_file &inp) {
	IBaseInteraction::get_settings(inp);

	getInputNumber(&inp, "PS_alpha", &_PS_alpha, 0);
	getInputInt(&inp, "PS_n", &_PS_n, 0);
	getInputString(&inp, "PS_bond_file", _bond_filename, 1);
	getInputBool(&inp, "PS_only_links_in_bondfile", &_only_links_in_bondfile, 0);

	for(int i = 0; i < 3; i++) {
		std::string key = Utils::sformat("PS_rfene[%d]", i);
		getInputNumber(&inp, key.c_str(), _rfene.data() + i, 0);

		key = Utils::sformat("PS_Kfene[%d]", i);
		getInputNumber(&inp, key.c_str(), _Kfene.data() + i, 0);

		key = Utils::sformat("PS_WCA_sigma[%d]", i);
		getInputNumber(&inp, key.c_str(), _WCA_sigma.data() + i, 0);
	}
}

void PolymerSwapInteraction::init() {
	std::transform(_rfene.begin(), _rfene.end(), _sqr_rfene.begin(), [](number r) {
		return SQR(r);
	});

	std::transform(_WCA_sigma.begin(), _WCA_sigma.end(), _PS_sqr_rep_rcut.begin(), [this](number sigma) {
		return pow(2. * sigma, 1. / this->_PS_n);
	});

	_rcut = sqrt(*std::max_element(_PS_sqr_rep_rcut.begin(), _PS_sqr_rep_rcut.end()));

	if(_PS_alpha > 0.) {
		if(_rfene[0] > _rcut) {
			_rcut = _rfene[0];
		}
	}
	_sqr_rcut = SQR(_rcut);

	OX_LOG(Logger::LOG_INFO, "PolymerSwap: total rcut: %lf (%lf)", _rcut, _sqr_rcut);

	if(_PS_alpha != 0) {
		if(_PS_alpha < 0.) {
			throw oxDNAException("MG_alpha may not be negative");
		}
		_PS_gamma = M_PI / (_sqr_rfene[0] - pow(2., 1. / 3.));
		_PS_beta = 2 * M_PI - _sqr_rfene[0] * _PS_gamma;
		OX_LOG(Logger::LOG_INFO, "MG: alpha = %lf, beta = %lf, gamma = %lf", _PS_alpha, _PS_beta, _PS_gamma);
	}
}

number PolymerSwapInteraction::_fene(BaseParticle *p, BaseParticle *q, bool update_forces) {
	number sqr_r = _computed_r.norm();

	int int_type = p->type + q->type;
	number sqr_rfene = _sqr_rfene[int_type];

	if(sqr_r > sqr_rfene) {
		if(update_forces) {
			throw oxDNAException("The distance between particles %d and %d (type: %d, r: %lf) exceeds the FENE distance (%lf)", p->index, q->index, int_type, sqrt(sqr_r), sqrt(sqr_rfene));
		}
		set_is_infinite(true);
		return 10e10;
	}

	number Kfene = _Kfene[int_type];
	number energy = -Kfene * sqr_rfene * log(1. - sqr_r / sqr_rfene);

	if(update_forces) {
		// this number is the module of the force over r, so we don't have to divide the distance
		// vector by its module
		number force_mod = -2 * Kfene * sqr_rfene / (sqr_rfene - sqr_r);
		p->force -= _computed_r * force_mod;
		q->force += _computed_r * force_mod;
	}

	return energy;
}

number PolymerSwapInteraction::_nonbonded(BaseParticle *p, BaseParticle *q, bool update_forces) {
	number sqr_r = _computed_r.norm();
	if(sqr_r > _sqr_rcut) {
		return (number) 0.;
	}

	int int_type = p->type + q->type;
	// sticky sites don't interact with nonbonded monomers
	if(!p->is_bonded(q) && int_type > 0) {
		return (number) 0.;
	}

	// this number is the module of the force over r, so we don't have to divide the distance
	// vector for its module
	number force_mod = 0;
	number energy = 0;
	// cut-off for all the repulsive interactions
	if(sqr_r < _PS_sqr_rep_rcut[int_type]) {
		number part = 1.;
		for(int i = 0; i < _PS_n / 2; i++) {
			part *= SQR(_WCA_sigma[int_type]) / sqr_r;
		}
		energy += 4. * (part * (part - 1.)) + 1. - _PS_alpha;
		if(update_forces) {
			force_mod += 4. * _PS_n * part * (2. * part - 1.) / sqr_r;
		}
	}
	// attraction
	else if(int_type == 0 && _PS_alpha != 0.) {
		energy += 0.5 * _PS_alpha * (cos(_PS_gamma * sqr_r + _PS_beta) - 1.0);
		if(update_forces) {
			force_mod += _PS_alpha * _PS_gamma * sin(_PS_gamma * sqr_r + _PS_beta);
		}
	}

	if(update_forces) {
		p->force -= _computed_r * force_mod;
		q->force += _computed_r * force_mod;
	}

	if(energy > 100) {
		printf("%d %d %lf %d\n", p->index, q->index, sqrt(sqr_r), int_type);
	}

	return energy;
}

number PolymerSwapInteraction::pair_interaction(BaseParticle *p, BaseParticle *q, bool compute_r, bool update_forces) {
	if(p->is_bonded(q)) {
		return pair_interaction_bonded(p, q, compute_r, update_forces);
	}
	else {
		return pair_interaction_nonbonded(p, q, compute_r, update_forces);
	}
}

number PolymerSwapInteraction::pair_interaction_bonded(BaseParticle *p, BaseParticle *q, bool compute_r, bool update_forces) {
	number energy = (number) 0.f;

	if(p->is_bonded(q)) {
		if(compute_r) {
			if(q != P_VIRTUAL && p != P_VIRTUAL) {
				_computed_r = _box->min_image(p->pos, q->pos);
			}
		}

		energy = _fene(p, q, update_forces);
		energy += _nonbonded(p, q, update_forces);
	}

	return energy;
}

number PolymerSwapInteraction::pair_interaction_nonbonded(BaseParticle *p, BaseParticle *q, bool compute_r, bool update_forces) {
	if(p->is_bonded(q)) {
		return (number) 0.f;
	}

	if(compute_r) {
		_computed_r = _box->min_image(p->pos, q->pos);
	}

	return _nonbonded(p, q, update_forces);
}

void PolymerSwapInteraction::check_input_sanity(std::vector<BaseParticle*> &particles) {
	for(uint i = 0; i < particles.size(); i++) {
		CustomParticle *p = static_cast<CustomParticle*>(particles[i]);
		for(typename std::set<CustomParticle*>::iterator it = p->bonded_neighs.begin(); it != p->bonded_neighs.end(); it++) {

		}
	}
}

void PolymerSwapInteraction::allocate_particles(std::vector<BaseParticle*> &particles) {
	for(uint i = 0; i < particles.size(); i++) {
		particles[i] = new CustomParticle();
	}
}

void PolymerSwapInteraction::read_topology(int *N_strands, std::vector<BaseParticle*> &particles) {
	std::string line;
	int N_from_conf = particles.size();
	IBaseInteraction::read_topology(N_strands, particles);

	std::ifstream topology;
	topology.open(_topology_filename, std::ios::in);
	if(!topology.good()) {
		throw oxDNAException("Can't read topology file '%s'. Aborting", _topology_filename);
	}
	std::getline(topology, line);
	std::vector<int> sticky_particles;
	while(topology.good()) {
		std::getline(topology, line);
		auto ps = Utils::getParticlesFromString(particles, line, "PolymerSwapInteraction");
		sticky_particles.insert(sticky_particles.begin(), ps.begin(), ps.end());
	}
	topology.close();

	OX_LOG(Logger::LOG_INFO, "PolymerSwap: %d sticky particles found in the topology", sticky_particles.size());

	std::ifstream bond_file(_bond_filename.c_str());

	if(!bond_file.good()) {
		throw oxDNAException("Can't read bond file '%s'. Aborting", _bond_filename.c_str());
	}
	// skip the headers and the particle positions if the user specified that the bondfile does not contain bonds only
	if(!_only_links_in_bondfile) {
		int to_skip = N_from_conf + 2;
		for(int i = 0; i < to_skip; i++) {
			std::getline(bond_file, line);
		}
		if(!bond_file.good()) {
			throw oxDNAException("The bond file '%s' does not contain the right number of lines. Aborting", _bond_filename.c_str());
		}
	}

	for(int i = 0; i < N_from_conf; i++) {
		std::getline(bond_file, line);
		auto line_spl = Utils::split(line, ' ');
		CustomParticle *p;
		int n_bonds;

		if(line_spl.size() == 2) {
			int idx = std::stoi(line_spl[0]);
			idx--;

			n_bonds = std::stoi(line_spl[1]);
			p = static_cast<CustomParticle*>(particles[idx]);

			if(i != idx) {
				throw oxDNAException("There is something wrong with the bond file. Expected index %d, found %d\n", i, idx);
			}
		}
		else {
			n_bonds = std::stoi(line_spl[0]);
			p = static_cast<CustomParticle*>(particles[i]);
		}

		p->type = MONOMER;
		if(std::find(sticky_particles.begin(), sticky_particles.end(), p->index) != sticky_particles.end()) {
			p->type = STICKY;
		}
		p->btype = n_bonds;
		p->strand_id = 0;
		p->n3 = p->n5 = P_VIRTUAL;
		for(int j = 0; j < n_bonds; j++) {
			int n_idx;
			bond_file >> n_idx;
			// the >> operator always leaves '\n', which is subsequently picked up by getline if
			// we don't explicitly ignore it
			bond_file.ignore();
			n_idx--;
			CustomParticle *q = static_cast<CustomParticle*>(particles[n_idx]);
			p->add_bonded_neigh(q);
		}
	}

	*N_strands = N_from_conf;
}

extern "C" PolymerSwapInteraction* make_PolymerSwapInteraction() {
	return new PolymerSwapInteraction();
}

