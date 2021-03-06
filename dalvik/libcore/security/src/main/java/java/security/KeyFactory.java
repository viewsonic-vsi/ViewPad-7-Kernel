/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package java.security;

import java.security.spec.InvalidKeySpecException;
import java.security.spec.KeySpec;

import org.apache.harmony.security.fortress.Engine;
import org.apache.harmony.security.internal.nls.Messages;

/**
 * {@code KeyFactory} is an engine class that can be used to translate between
 * public and private key objects and convert keys between their external
 * representation, that can be easily transported and their internal
 * representation.
 */
public class KeyFactory {
    // The service name.
    private static final String SERVICE = "KeyFactory"; //$NON-NLS-1$
    
    // The provider
    private Provider provider;
    
    
    // Used to access common engine functionality
    static private Engine engine = new Engine(SERVICE);
    
    // The SPI implementation.
    private KeyFactorySpi spiImpl; 
    
    // The algorithm.
    private String algorithm;

    /**
     * Constructs a new instance of {@code KeyFactory} with the specified
     * arguments.
     * 
     * @param keyFacSpi
     *            the concrete key factory service.
     * @param provider
     *            the provider.
     * @param algorithm
     *            the algorithm to use.
     */
    protected KeyFactory(KeyFactorySpi keyFacSpi, 
                         Provider provider,
                         String algorithm) {
        this.provider = provider;
        this. algorithm = algorithm;
        this.spiImpl = keyFacSpi;
    }

    /**
     * Returns a new instance of {@code KeyFactory} that utilizes the specified
     * algorithm.
     * 
     * @param algorithm
     *            the name of the algorithm.
     * @return a new instance of {@code KeyFactory} that utilizes the specified
     *         algorithm.
     * @throws NoSuchAlgorithmException
     *             if no provider provides the requested algorithm.
     */
    public static KeyFactory getInstance(String algorithm)
                                throws NoSuchAlgorithmException {
        if (algorithm == null) {
            throw new NullPointerException(Messages.getString("security.01")); //$NON-NLS-1$
        }
        synchronized (engine) {
            engine.getInstance(algorithm, null);
            return new KeyFactory((KeyFactorySpi)engine.spi, engine.provider, algorithm);
        }
    }

    /**
     * Returns a new instance of {@code KeyFactory} that utilizes the specified
     * algorithm from the specified provider.
     * 
     * @param algorithm
     *            the name of the algorithm.
     * @param provider
     *            the name of the provider.
     * @return a new instance of {@code KeyFactory} that utilizes the specified
     *         algorithm from the specified provider.
     * @throws NoSuchAlgorithmException
     *             if the provider does not provide the requested algorithm.
     * @throws NoSuchProviderException
     *             if the requested provider is not available.
     * @throws IllegalArgumentException
     *             if {@code provider} is {@code null} or empty.
     */
    @SuppressWarnings("nls")
    public static KeyFactory getInstance(String algorithm, String provider)
                                throws NoSuchAlgorithmException, NoSuchProviderException {
        if ((provider == null) || (provider.length() == 0)) {
            throw new IllegalArgumentException(Messages.getString("security.02"));
        }
        Provider p = Security.getProvider(provider);
        if (p == null) {
            throw new NoSuchProviderException(Messages.getString("security.03", provider));
        }
        return getInstance(algorithm, p);    
    }

    /**
     * Returns a new instance of {@code KeyFactory} that utilizes the specified
     * algorithm from the specified provider.
     * 
     * @param algorithm
     *            the name of the algorithm.
     * @param provider
     *            the security provider.
     * @return a new instance of {@code KeyFactory} that utilizes the specified
     *         algorithm from the specified provider.
     * @throws NoSuchAlgorithmException
     *             if the provider does not provide the requested algorithm.
     */
    public static KeyFactory getInstance(String algorithm, Provider provider)
                                 throws NoSuchAlgorithmException {
        if (provider == null) {
            throw new IllegalArgumentException(Messages.getString("security.04")); //$NON-NLS-1$
        }
        if (algorithm == null) {
            throw new NullPointerException(Messages.getString("security.01")); //$NON-NLS-1$
        }
        synchronized (engine) {
            engine.getInstance(algorithm, provider, null);
            return new KeyFactory((KeyFactorySpi)engine.spi, provider, algorithm);
        }
    }

    /**
     * Returns the provider associated with this {@code KeyFactory}.
     * 
     * @return the provider associated with this {@code KeyFactory}.
     */
    public final Provider getProvider() {
        return provider;
    }

    /**
     * Returns the name of the algorithm associated with this {@code
     * KeyFactory}.
     * 
     * @return the name of the algorithm associated with this {@code
     *         KeyFactory}.
     */
    public final String getAlgorithm() {
        return algorithm;
    }

    /**
     * Generates a instance of {@code PublicKey} from the given key
     * specification.
     * 
     * @param keySpec
     *            the specification of the public key
     * @return the public key
     * @throws InvalidKeySpecException
     *             if the specified {@code keySpec} is invalid
     */
    public final PublicKey generatePublic(KeySpec keySpec)
                                throws InvalidKeySpecException {
        return spiImpl.engineGeneratePublic(keySpec);
    }

    /**
     * Generates a instance of {@code PrivateKey} from the given key
     * specification.
     * 
     * @param keySpec
     *            the specification of the private key.
     * @return the private key.
     * @throws InvalidKeySpecException
     *             if the specified {@code keySpec} is invalid.
     */
    public final PrivateKey generatePrivate(KeySpec keySpec)
                                throws InvalidKeySpecException {
        return spiImpl.engineGeneratePrivate(keySpec);
    }

    /**
     * Returns the key specification for the specified key.
     * 
     * @param key
     *            the key from which the specification is requested.
     * @param keySpec
     *            the type of the requested {@code KeySpec}.
     * @return the key specification for the specified key.
     * @throws InvalidKeySpecException
     *             if the key can not be processed, or the requested requested
     *             {@code KeySpec} is inappropriate for the given key.
     */
    public final <T extends KeySpec> T getKeySpec(Key key,
                                    Class<T> keySpec)
                            throws InvalidKeySpecException {
        return spiImpl.engineGetKeySpec(key, keySpec);
    }

    /**
     * Translates the given key into a key from this key factory.
     * 
     * @param key
     *            the key to translate.
     * @return the translated key.
     * @throws InvalidKeyException
     *             if the specified key can not be translated by this key
     *             factory.
     */
    public final Key translateKey(Key key)
                        throws InvalidKeyException {
        return spiImpl.engineTranslateKey(key);
    }
}
