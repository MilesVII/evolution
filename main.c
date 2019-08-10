#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "drawing.h"
#include "utils.h"
#include "io.h"

#define getCell(a, x, y, w) (a + (w * y + x))
#define bitAt(x, y) (x & (1 << y))
int min(int a, int b){
	if (a < b)
		return a;
	else
		return b;
}
int max(int a, int b){
	if (a > b)
		return a;
	else
		return b;
}

float frand(){
	return (double)rand() / (double)RAND_MAX;
}

uint32_t cWhite = 0xFFFFFF00;
uint32_t cBlack = 0x00000000;

void mutate(uint32_t* color){
	*color ^= (1 << (rand() % 24 + 8));
}
uint32_t mix(uint32_t* colorA, uint32_t* colorB){
	uint32_t mixed = 0;
	uint32_t random = rand();
	for (int i = 0; i < sizeof(uint32_t) * 8; ++i)
		mixed += bitAt(random, i) ? bitAt(*colorA, i) : bitAt(*colorB, i);

	return mixed;
}
double colorDistance(uint32_t colorA, uint32_t colorB){
	int rd = decodeColorR(colorA) - decodeColorR(colorB);
	int gd = decodeColorG(colorA) - decodeColorG(colorB);
	int bd = decodeColorB(colorA) - decodeColorB(colorB);
	return sqrt(rd * rd + gd * gd + bd * bd);
}
double colorSimilarity(uint32_t colorA, uint32_t colorB){
	return 1.0 - colorDistance(colorA, colorB) / colorDistance(cWhite, cBlack);
}

//const int MAX_POPULATION = 4096;
#define MAX_POPULATION 2048
struct Configuration {
	int satietyLimit;
	int grassSatietyPoints;
	int meatSatietyPoints;
	int hungerRate;
	int octocatOptimalPopulation;
	int raptorOptimalPopulation;
	int grassLimit;
	int grassGrowthRate;
	float octocatProcreateChance;
	float octocatMutationChance;
	float raptorProcreateChance;
	float raptorMutationChance;
	//bool hardwareRender;
	int guiFieldWidth;
	int guiFieldHeight;
	int guiWindowWidth;
	int guiWindowHeight;
	float guiCreatureBodyRadius;
	float guiCreatureOutlineRadius;
	uint32_t guiRaptorOutline;
	uint32_t guiOctocatOutline;
} configuration;
typedef struct Configuration Configuration;

void initConfigurationWithDefault(Configuration* configuration){
	configuration->satietyLimit = 32;
	configuration->grassSatietyPoints = 4;
	configuration->meatSatietyPoints = 12;
	configuration->hungerRate = 2;
	configuration->octocatOptimalPopulation = 6;
	configuration->raptorOptimalPopulation = 6;
	configuration->grassLimit = 64;
	configuration->grassGrowthRate = configuration->grassSatietyPoints;
	configuration->octocatProcreateChance = .64;
	configuration->octocatMutationChance = .2;
	configuration->raptorProcreateChance = .64;
	configuration->raptorMutationChance = .2;
	//configuration->hardwareRender = false;
	configuration->guiFieldWidth = 320;
	configuration->guiFieldHeight = 320;
	configuration->guiWindowWidth = configuration->guiFieldWidth;
	configuration->guiWindowHeight = configuration->guiFieldHeight;
	configuration->guiCreatureBodyRadius = 3.0;
	configuration->guiCreatureOutlineRadius = 4.0;
	configuration->guiRaptorOutline = 0xFF000000;
	configuration->guiOctocatOutline = 0x00FF0000;
}

const int DIRECTIONS[8] = {1, 0, -1, 0, 0, 1, 0, -1};

struct Creature {
	bool alive;
	bool mewborn;
	int x, y;
	uint32_t color;
	int satiety;
};
typedef struct Creature Creature;
Creature** creaturesAtCellArray;
int creaturesAtCellLength;

struct SimulationSeed {
	int grassAmount;
	int octocatsPopulation;
	int raptorsPopulation;
	int satiety;
	uint32_t octocatColor;
	uint32_t raptorColor;
} defaultSeed;
typedef struct SimulationSeed SimulationSeed;

struct Simulation {
	int w, h;
	int* grass;
	Creature octocats[MAX_POPULATION];
	Creature raptors[MAX_POPULATION];
	uint32_t backgroundColor;
} mainSimulation;
typedef struct Simulation Simulation;

Creature** getCreaturesAtCell(Creature* population, int x, int y, Creature* except){
	int j = 0;
	for (int i = 0; i < MAX_POPULATION; ++i){
		Creature* c = population + i;
		if (c != except && c->alive && c->x == x && c->y == y)
			creaturesAtCellArray[j++] = c;
	}

	creaturesAtCellLength = j;
	return creaturesAtCellArray;
}

Creature* createCreature(Simulation* sim, bool isOctocat, int x, int y, uint32_t color){
	Creature* mewborn = NULL;
	Creature* population = isOctocat ? sim->octocats : sim->raptors;
	for (int i = 0; i < MAX_POPULATION; ++i)
		if (!population[i].alive){
			mewborn = &population[i];
			break;
		}
	
	if (mewborn == NULL)
		return NULL;

	mewborn->alive = true;
	mewborn->color = color;
	mewborn->satiety = configuration.grassSatietyPoints;
	mewborn->x = x;
	mewborn->y = y;
	mewborn->mewborn = true;

	if (frand() < isOctocat ? configuration.octocatMutationChance : configuration.raptorMutationChance)
		mutate(&mewborn->color);

	return mewborn;
}

void resetMewborns(struct Simulation* sim){
	for (int i = 0; i < MAX_POPULATION; ++i){
		sim->octocats[i].mewborn = false;
		sim->raptors[i].mewborn = false;
	}
}

void initSimulation(Simulation* sim, int w, int h, uint32_t backgroundColor, SimulationSeed* seed){
	sim->w = w;
	sim->h = h;
	sim->backgroundColor = backgroundColor;
	int xx, yy;
	xx = rand() % w;
	yy = rand() % h;

	sim->grass = malloc(w * h * sizeof(int));
	for (int i = 0; i < w * h; ++i)
		sim->grass[i] = seed->grassAmount;
	
	for (int i = 0; i < MAX_POPULATION; ++i){
		sim->octocats[i].alive = i < seed->octocatsPopulation;
		sim->octocats[i].mewborn = false;
		sim->raptors[i].alive = i < seed->raptorsPopulation;
		sim->raptors[i].mewborn = false;
	}
	for (int i = 0; i < seed->octocatsPopulation; ++i){
		sim->octocats[i].x = xx;//rand() % w;
		sim->octocats[i].y = yy;//rand() % h;
		sim->octocats[i].color = seed->octocatColor;//~sim->backgroundColor;
		sim->octocats[i].satiety = seed->satiety;
	}
	xx = rand() % w;
	yy = rand() % h;
	for (int i = 0; i < seed->raptorsPopulation; ++i){
		sim->raptors[i].x = xx;
		sim->raptors[i].y = yy;
		sim->raptors[i].color = seed->raptorColor;//~sim->backgroundColor;
		sim->raptors[i].satiety = seed->satiety;
	}
}

void fucc(Simulation* sim, bool isOctocat, Creature* cat, Creature** neighbours, int neighboursCount){
	float procreateChance = isOctocat ? configuration.octocatProcreateChance : configuration.raptorProcreateChance;
	if (neighboursCount > 0 && frand() <= procreateChance){
		Creature* mate = *(neighbours + rand() % neighboursCount);
		createCreature(sim, isOctocat, cat->x, cat->y, mix(&cat->color, &mate->color));
	}
}

int getCellDesirabilityForOctocat(Simulation* sim, Creature* cat, int x, int y){
	int desired = 0;
	if (*getCell(sim->grass, x, y, sim->w) >= configuration.grassSatietyPoints)
		desired += 2;
	
	getCreaturesAtCell(sim->octocats, x, y, NULL);

	if (creaturesAtCellLength > configuration.octocatOptimalPopulation){
		desired -= 1;
	} else if (creaturesAtCellLength < configuration.octocatOptimalPopulation){
		desired += 1;
	}

	return desired;
}
int getCellDesirabilityForRaptor(Simulation* sim, Creature* cat, int x, int y){
	int desired = 0;
	
	getCreaturesAtCell(sim->octocats, x, y, NULL);
	if (creaturesAtCellLength > configuration.raptorOptimalPopulation - 1)
		desired += 3;
	
	getCreaturesAtCell(sim->raptors, x, y, NULL);
	if (creaturesAtCellLength > configuration.raptorOptimalPopulation){
		desired -= 2;
	} else if (creaturesAtCellLength < configuration.raptorOptimalPopulation){
		desired += 4;
	}

	return desired;
}

void migrate(Simulation* sim, Creature* creature, bool isOctocat){
	int mostDesiredDestination = 0;
	int mostDesiredPoints = -100;
	int desirability[4];
	int cx, cy;
	cx = creature->x;
	cy = creature->y;
	for (int j = 0; j < 8; j += 2){
		desirability[j / 2] = -101;
		int x = *(DIRECTIONS + j);
		int y = *(DIRECTIONS + j + 1);
		if (cx + x < 0 || cy + y < 0 || cx + x >= sim->w || cy + y >= sim->h)
			continue;
		
		int desired;
		if (isOctocat)
			desired = getCellDesirabilityForOctocat(sim, creature, cx + x, cy + y);
		else
			desired = getCellDesirabilityForRaptor(sim, creature, cx + x, cy + y);
		
		desirability[j / 2] = desired;
		if (desired > mostDesiredPoints){
			mostDesiredDestination = j;
			mostDesiredPoints = desired;
		}
	}
	int mostDesiredDestinationX;
	int maxPool = 0;
	for (int i = 0; i < 4; ++i)
		if (desirability[i] == mostDesiredPoints)
			++maxPool;
	if (maxPool > 1){
		int selectedPoolDirection = rand() % maxPool;
		for (int i = 0, j = 0; i < 4; ++i)
			if ((desirability[i] == mostDesiredPoints) && (selectedPoolDirection == j++)){
				mostDesiredDestinationX = mostDesiredDestination;
				mostDesiredDestination = i * 2;
			}
	}

	if (mostDesiredPoints != -100){
		creature->x += DIRECTIONS[mostDesiredDestination];
		creature->y += DIRECTIONS[mostDesiredDestination + 1];
	}
}

void tick(Simulation* sim){
	for (int i = 0; i < MAX_POPULATION; ++i){
		Creature* c = &sim->octocats[i];
		int redTractorPoint = 0; //max is 4
		if (!c->alive || c->mewborn){
			c->mewborn = false;
			continue;
		}

		Creature** neighbours = getCreaturesAtCell(sim->octocats, c->x, c->y, c);
		int neighboursCount = creaturesAtCellLength;
		if (neighboursCount > configuration.octocatOptimalPopulation - 1)
			redTractorPoint += 2; //Run if too close
		
		//eat
		int* grass = getCell(sim->grass, c->x, c->y, sim->w);
		c->satiety -= configuration.hungerRate;
		if (c->satiety < configuration.satietyLimit - configuration.grassSatietyPoints){
			if (*grass < configuration.grassSatietyPoints){
				if (*grass <= 0)
					++redTractorPoint; //Run if communism
				c->satiety += *grass;
				*grass = 0;
				++redTractorPoint; //Run if no food
			} else {
				c->satiety += configuration.grassSatietyPoints;
				*grass -= configuration.grassSatietyPoints;
			}
		}
		if (c->satiety <= 0){
			c->alive = false;
			continue;
		}

		//fucc
		if (neighboursCount < configuration.octocatOptimalPopulation + 2 && c->satiety >= configuration.satietyLimit / 2)
			fucc(sim, true, c, neighbours, neighboursCount);

		//migrate
		if (rand() % 4 < redTractorPoint){
			migrate(sim, c, true);
		}
	}

	for (int i = 0; i < MAX_POPULATION; ++i){
		struct Creature* r = &sim->raptors[i];
		int redTractorPoint = 0; //max is 4
		if (!r->alive || r->mewborn){
			r->mewborn = false;
			continue;
		}

		//hunt
		r->satiety -= configuration.hungerRate;
		Creature** prey = getCreaturesAtCell(sim->octocats, r->x, r->y, NULL);
		int preyCount = creaturesAtCellLength;
		if (r->satiety < (configuration.satietyLimit - configuration.meatSatietyPoints) && preyCount > 0){
			Creature* victim = *(prey + rand() % preyCount);
			float chanceToCatch = frand() * (1.0 - colorSimilarity(victim->color, sim->backgroundColor));
			chanceToCatch *= colorSimilarity(r->color, victim->color);
			if (frand() < chanceToCatch){
				victim->alive = false;
				r->satiety += configuration.meatSatietyPoints;
			} else {
				redTractorPoint += 1; //Run if prey is hard to catch
			}
		} else {
			redTractorPoint += 2; //Run if no prey
		}
		if (r->satiety <= configuration.satietyLimit / 2)
			redTractorPoint += 1; //Run if hungry
		
		if (r->satiety <= 0){
			r->alive = false;
			continue;
		}

		Creature** neighbours = getCreaturesAtCell(sim->raptors, r->x, r->y, r);
		int neighboursCount = creaturesAtCellLength;
		if (neighboursCount > configuration.raptorOptimalPopulation - 1)
			redTractorPoint += 1; //Run if too close
		
		//fucc
		if (neighboursCount < configuration.raptorOptimalPopulation + 2 && r->satiety >= configuration.satietyLimit / 2)
			fucc(sim, false, r, neighbours, neighboursCount);
		
		//migrate;
		if (rand() % 4 < redTractorPoint){
			migrate(sim, r, false);
		}
	}
	
	for (int i = 0; i < sim->w * sim->h; ++i)
		sim->grass[i] = min(sim->grass[i] + configuration.grassGrowthRate, configuration.grassLimit);

	resetMewborns(sim);
}

int countOctocats(Simulation* sim){
	int count = 0;
	for (int i = 0; i < MAX_POPULATION; ++i)
		if (sim->octocats[i].alive)
			++count;
	return count;
}
int countRaptors(Simulation* sim){
	int count = 0;
	for (int i = 0; i < MAX_POPULATION; ++i)
		if (sim->raptors[i].alive)
			++count;
	return count;
}
float getAverageFitness(Simulation* sim){
	float count = 0;
	float fitness = 0;
	for (int i = 0; i < MAX_POPULATION; ++i)
		if (sim->octocats[i].alive){
			++count;
			fitness += colorSimilarity(sim->backgroundColor, sim->octocats[i].color);
		}
	
	return fitness / count;
}

void renderCircle(int tx, int ty, float radius, uint32_t color){
	drawing_setColor(color);
	int xfrom, xto, yfrom, yto;
	xfrom = max(0, tx - (int)ceil(radius));
	xto = min(configuration.guiFieldWidth, tx + (int)ceil(radius));
	yfrom = max(0, ty - (int)ceil(radius));
	yto = min(configuration.guiFieldHeight, ty + (int)ceil(radius));
	for (int x = xfrom; x < xto; ++x)
	for (int y = yfrom; y < yto; ++y){
		int dx = x - tx;
		int dy = y - ty;
		if (dx * dx + dy * dy < radius * radius){
			drawing_pixel(x, y);
		}
	}
}

void drawTick(Simulation* sim, float innerRadius, float outerRadius){
	int cellW = configuration.guiFieldWidth / sim->w;
	int cellH = configuration.guiFieldHeight / sim->h;
	for (int i = 0; i < MAX_POPULATION; ++i){
		Creature* c = &sim->octocats[i];
		if (c->alive){
			int x = cellW * c->x + (rand() % cellW);
			int y = cellH * c->y + (rand() % cellH);
			renderCircle(x, y, outerRadius, configuration.guiOctocatOutline);
			renderCircle(x, y, innerRadius, c->color);
		}
		c = &sim->raptors[i];
		if (c->alive){
			int x = cellW * c->x + (rand() % cellW);
			int y = cellH * c->y + (rand() % cellH);
			renderCircle(x, y, outerRadius, configuration.guiRaptorOutline);
			renderCircle(x, y, innerRadius, c->color);
		}
	}
}

int main(int argc, char* args[]) {
	srand(time(NULL));
	creaturesAtCellArray = malloc(MAX_POPULATION * sizeof(Creature*));

	initConfigurationWithDefault(&configuration);

	defaultSeed.grassAmount = configuration.grassLimit;
	defaultSeed.satiety = configuration.satietyLimit;
	defaultSeed.octocatsPopulation = 512;
	defaultSeed.raptorsPopulation = 128;
	defaultSeed.octocatColor = 0xFFFFFFFF;
	defaultSeed.raptorColor = 0xFFFFFFFF;

	initSimulation(&mainSimulation, 16, 16, cBlack, &defaultSeed);

	int ticks = 0;
	int octocats = -1;
	int raptors = -1;

	drawing_init("SES 2.0", configuration.guiWindowWidth, configuration.guiWindowHeight);
	io_init();

	drawing_clear(mainSimulation.backgroundColor);
	drawTick(&mainSimulation, configuration.guiCreatureBodyRadius, configuration.guiCreatureOutlineRadius);
	drawing_render();

	bool quit = false;
	
	while(!quit) {
		int mouseX, mouseY;
		io_getMouse(&mouseX, &mouseY);
		

		switch (io_getEvent())
		{
		case IO_QUIT:
			quit = true;
			break;
		case IO_CLICK:
			initSimulation(&mainSimulation, mainSimulation.w, mainSimulation.h, mainSimulation.backgroundColor, &defaultSeed);
			raptors = 1;
			break;
		default:
			break;
		}

		if (raptors == 0)
			continue;
		
		drawing_clear(mainSimulation.backgroundColor);
		tick(&mainSimulation);
		drawTick(&mainSimulation, configuration.guiCreatureBodyRadius, configuration.guiCreatureOutlineRadius);
		drawing_render();

		++ticks;
		octocats = countOctocats(&mainSimulation);
		raptors = countRaptors(&mainSimulation);
		float catFitness = getAverageFitness(&mainSimulation);
		printf("Tick #%d\nOctocats: %d\nRaptors: %d\nAverage octocat fitness: %.2f%%\n\n", ticks, octocats, raptors, catFitness * 100.0);
	}

	drawing_terminate();

	free(mainSimulation.grass);
	free(creaturesAtCellArray);
	return 0;
}
