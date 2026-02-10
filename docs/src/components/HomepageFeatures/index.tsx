import type {ReactNode} from 'react';
import clsx from 'clsx';
import Heading from '@theme/Heading';
import styles from './styles.module.css';

type FeatureItem = {
  title: string;
  description: ReactNode;
};

const FeatureList: FeatureItem[] = [
  {
    title: 'Entity Component System',
    description: (
      <>
        Bevy-inspired ECS with a C# World, typed components, system queries,
        and frame-ordered system execution. Build game logic in pure C#.
      </>
    ),
  },
  {
    title: 'Vulkan Rendering',
    description: (
      <>
        C++ Vulkan backend with glTF mesh loading, multi-entity draw calls,
        push-constant transforms, and Blinn-Phong shading with up to 8 dynamic lights.
      </>
    ),
  },
  {
    title: 'C# / C++ Bridge',
    description: (
      <>
        Mono P/Invoke bridge connects C# game logic to C++ rendering.
        Full control over cameras, input, lighting, and timing from managed code.
      </>
    ),
  },
];

function Feature({title, description}: FeatureItem) {
  return (
    <div className={clsx('col col--4')}>
      <div className="text--center padding-horiz--md padding-vert--md">
        <Heading as="h3">{title}</Heading>
        <p>{description}</p>
      </div>
    </div>
  );
}

export default function HomepageFeatures(): ReactNode {
  return (
    <section className={styles.features}>
      <div className="container">
        <div className="row">
          {FeatureList.map((props, idx) => (
            <Feature key={idx} {...props} />
          ))}
        </div>
      </div>
    </section>
  );
}
